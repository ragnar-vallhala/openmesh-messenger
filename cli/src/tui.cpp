#include "tui.hpp"

#include "openmesh/core/hex.hpp"
#include "openmesh/crypto/sign.hpp"
#include "openmesh/messaging/engine.hpp"
#include "openmesh/net/signaling_client.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <clocale>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using openmesh::crypto::Identity;
using openmesh::messaging::Bytes;
using openmesh::messaging::Engine;
using openmesh::net::Endpoint;
using openmesh::net::SignalingClient;
using openmesh::storage::TrustStatus;

namespace omchat {
namespace {

Bytes to_bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}
std::string to_str(const Bytes& b) {
    return std::string(b.begin(), b.end());
}

std::optional<Bytes> from_hex(const std::string& hex) {
    if (hex.size() % 2 != 0)
        return std::nullopt;
    Bytes out;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        int hi = nib(hex[i]), lo = nib(hex[i + 1]);
        if (hi < 0 || lo < 0)
            return std::nullopt;
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return out;
}

std::string fmt_time(std::int64_t ts) {
    if (ts == 0)
        return "";
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::time_t now = std::time(nullptr);
    std::tm nowtm{};
    localtime_r(&now, &nowtm);
    char buf[16];
    if (tm.tm_yday == nowtm.tm_yday && tm.tm_year == nowtm.tm_year) {
        std::strftime(buf, sizeof(buf), "%H:%M", &tm);
    } else {
        std::strftime(buf, sizeof(buf), "%b %d", &tm);
    }
    return buf;
}

// Color pairs.
enum { CP_HEADER = 1, CP_SELECTED, CP_YOU, CP_ACCENT, CP_DIM };

class Tui {
public:
    Tui(Identity self, std::optional<Endpoint> server, std::optional<Endpoint> relay,
        std::string name, std::string store_path, std::string store_pass)
        : self_(std::move(self)), relay_(relay), name_(std::move(name)),
          store_path_(std::move(store_path)), store_pass_(std::move(store_pass)),
          engine_(self_.public_key, self_.secret_key) {
        if (server) {
            sig_ =
                std::make_unique<SignalingClient>(engine_.transport(), *server, self_.public_key,
                                                  [sk = self_.secret_key](const Bytes& c) {
                                                      return openmesh::crypto::sign_detached(c, sk);
                                                  });
        }
    }

    int run() {
        // Backend bring-up (before ncurses so errors print normally).
        if (!engine_.bind(Endpoint{"0.0.0.0", 0})) {
            std::fputs("error: could not bind a local socket\n", stderr);
            return 1;
        }
        if (!store_path_.empty() && !store_pass_.empty()) {
            if (engine_.open_store(store_path_, store_pass_)) {
                flash_ = "loaded saved chats";
            }
        }
        if (sig_) {
            flash_ = sig_->register_self() ? "registered — online" : "registration failed";
        }
        if (relay_) {
            engine_.set_relay(*relay_);
            engine_.announce_to(*relay_);
        }
        std::thread net([this] { net_loop(); });

        init_ui();
        ui_loop();
        endwin();

        running_ = false;
        net.join();
        return 0;
    }

private:
    // ---------------- backend (network thread) ----------------
    void submit(const std::string& cmd) {
        std::lock_guard<std::mutex> lk(qmtx_);
        queue_.push_back(cmd);
    }

    void net_loop() {
        auto last_hb = std::chrono::steady_clock::now();
        auto last_ann = last_hb;
        while (running_) {
            {
                std::deque<std::string> cmds;
                {
                    std::lock_guard<std::mutex> lk(qmtx_);
                    cmds.swap(queue_);
                }
                std::lock_guard<std::mutex> lk(mtx_);
                for (auto& c : cmds)
                    exec(c);
                while (engine_.poll(0)) {
                }
                engine_.tick();
                auto now = std::chrono::steady_clock::now();
                if (sig_ && now - last_hb > std::chrono::seconds(30)) {
                    sig_->heartbeat();
                    last_hb = now;
                }
                if (relay_ && now - last_ann > std::chrono::seconds(20)) {
                    engine_.announce_to(*relay_);
                    last_ann = now;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    // Runs on the network thread with mtx_ held.
    void exec(const std::string& line) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd == "/quit") {
            running_ = false;
        } else if (cmd == "/id") {
            flash_ = "your key: " + openmesh::core::to_hex(self_.public_key);
        } else if (cmd == "/add") {
            std::string nm, pk;
            in >> nm >> pk;
            do_add(nm, pk);
        } else if (cmd == "/accept" || cmd == "/reject") {
            std::string pkhex;
            in >> pkhex;
            auto pk = from_hex(pkhex);
            if (pk)
                do_decide(*pk, cmd == "/accept");
        } else if (cmd == "/send") {
            std::string pkhex;
            in >> pkhex;
            std::string text;
            std::getline(in, text);
            if (!text.empty() && text[0] == ' ')
                text.erase(0, 1);
            auto pk = from_hex(pkhex);
            if (pk) {
                if (!engine_.send(*pk, to_bytes(text)))
                    flash_ = "not delivered (offline?)";
            }
        }
    }

    void do_add(const std::string& nm, const std::string& pkhex) {
        auto pk = from_hex(pkhex);
        if (nm.empty() || !pk || pk->size() != 32) {
            flash_ = "usage: /add <name> <64-hex pubkey>";
            return;
        }
        engine_.set_nickname(*pk, nm);
        if (sig_) {
            auto found = sig_->discover(*pk);
            if (!found.endpoint) {
                flash_ = found.responded ? nm + " is offline" : "no signaling response";
                return;
            }
            auto active = engine_.connect(*pk, *found.endpoint);
            if (auto mine = sig_->public_endpoint())
                sig_->send_connect(*pk, to_bytes(mine->to_string()));
            flash_ = engine_.send_contact_request(*pk, active, to_bytes(name_))
                         ? "request sent to " + nm
                         : "could not send request";
        } else if (relay_) {
            flash_ = engine_.send_contact_request(*pk, *relay_, to_bytes(name_))
                         ? "request sent to " + nm + " via relay"
                         : "could not send request";
        }
    }

    void do_decide(const Bytes& pk, bool accept) {
        bool ok = accept ? engine_.accept_contact(pk) : engine_.reject_contact(pk);
        flash_ = ok ? (accept ? "accepted" : "rejected") : "no pending request";
    }

    // ---------------- model snapshot ----------------
    struct Item {
        bool is_request = false;
        Bytes pk;
        std::string name;
        std::string subtitle;
        std::int64_t sort_key = 0;
    };
    struct Line {
        std::string time;
        std::string who;
        std::string text;
        bool outgoing = false;
    };

    std::string name_of(const openmesh::storage::Contact& c) {
        if (!c.nickname.empty())
            return c.nickname;
        return openmesh::core::to_hex(c.public_key).substr(0, 8);
    }

    // Builds items_/conv_/status_ under mtx_. Runs on the UI thread.
    void build_snapshot() {
        std::lock_guard<std::mutex> lk(mtx_);
        items_.clear();
        std::vector<Item> chats;
        for (const auto& c : engine_.contacts()) {
            if (c.trust != TrustStatus::Accepted)
                continue;
            auto conv = engine_.conversation(c.public_key);
            Item it;
            it.pk = c.public_key;
            it.name = name_of(c);
            it.sort_key = conv.empty() ? 0 : conv.back().timestamp;
            it.subtitle =
                conv.empty() ? "" : (conv.back().outgoing ? "you: " : "") + conv.back().text;
            chats.push_back(std::move(it));
        }
        std::sort(chats.begin(), chats.end(),
                  [](const Item& a, const Item& b) { return a.sort_key > b.sort_key; });
        n_chats_ = chats.size();
        for (auto& c : chats)
            items_.push_back(std::move(c));

        for (const auto& q : engine_.pending_requests()) {
            Item it;
            it.is_request = true;
            it.pk = q.peer;
            auto c = engine_.contacts();
            it.name = openmesh::core::to_hex(q.peer).substr(0, 8);
            for (const auto& cc : c)
                if (cc.public_key == q.peer && !cc.nickname.empty())
                    it.name = cc.nickname;
            it.subtitle = q.greeting;
            items_.push_back(std::move(it));
        }

        if (selected_ >= static_cast<int>(items_.size()))
            selected_ = static_cast<int>(items_.size()) - 1;
        if (selected_ < 0)
            selected_ = 0;

        conv_.clear();
        status_.clear();
        if (selected_ >= 0 && selected_ < static_cast<int>(items_.size())) {
            const Item& sel = items_[static_cast<std::size_t>(selected_)];
            if (sel.is_request) {
                status_ = "request from " + sel.name;
                conv_.push_back(
                    {"", sel.name, sel.subtitle.empty() ? "(no greeting)" : sel.subtitle, false});
                conv_.push_back({"", "", "", false});
                conv_.push_back({"", "", "Type /accept or /reject (or press a / r).", false});
            } else {
                bool direct = engine_.has_direct_path(sel.pk);
                status_ = sel.name + (direct ? "  ● direct" : (relay_ ? "  ○ via relay" : ""));
                for (const auto& m : engine_.conversation(sel.pk)) {
                    conv_.push_back(
                        {fmt_time(m.timestamp), m.outgoing ? "you" : sel.name, m.text, m.outgoing});
                }
            }
        }
    }

    // ---------------- UI (main thread) ----------------
    void init_ui() {
        std::setlocale(LC_ALL, "");
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        timeout(120);
        curs_set(1);
        if (has_colors()) {
            start_color();
            use_default_colors();
            init_pair(CP_HEADER, COLOR_CYAN, -1);
            init_pair(CP_SELECTED, COLOR_BLACK, COLOR_CYAN);
            init_pair(CP_YOU, COLOR_GREEN, -1);
            init_pair(CP_ACCENT, COLOR_YELLOW, -1);
            init_pair(CP_DIM, COLOR_WHITE, -1);
        }
    }

    void ui_loop() {
        while (running_) {
            build_snapshot();
            render();
            int ch = getch();
            if (ch != ERR)
                on_key(ch);
        }
    }

    static std::string clip(const std::string& s, int w) {
        if (w <= 0)
            return "";
        if (static_cast<int>(s.size()) <= w)
            return s;
        if (w <= 1)
            return s.substr(0, static_cast<std::size_t>(w));
        return s.substr(0, static_cast<std::size_t>(w - 1)) + "…";
    }

    void render() {
        erase();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        int sw = std::min(30, cols / 3);
        if (sw < 18)
            sw = std::min(18, cols);
        const int top = 1, bottom = rows - 3; // content region [top, bottom]

        // Status bar.
        attron(COLOR_PAIR(CP_SELECTED));
        mvhline(0, 0, ' ', cols);
        std::string fp = self_.fingerprint().substr(0, 10);
        mvaddstr(0, 1, clip("OpenMesh  " + name_ + "  " + fp + "…", cols - 2).c_str());
        std::string right = status_.empty() ? "" : status_;
        mvaddstr(0, std::max(1, cols - 1 - static_cast<int>(right.size())),
                 clip(right, cols / 2).c_str());
        attroff(COLOR_PAIR(CP_SELECTED));

        // Sidebar separator.
        for (int r = top; r <= bottom; ++r)
            mvaddch(r, sw, ACS_VLINE);

        // Sidebar contents.
        int y = top;
        attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
        mvaddstr(y++, 0, clip(" CHATS", sw).c_str());
        attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
        if (n_chats_ == 0 && y <= bottom) {
            attron(A_DIM);
            mvaddstr(y++, 0, clip("  none yet — /add", sw).c_str());
            attroff(A_DIM);
        }
        for (int i = 0; i < n_chats_ && y <= bottom; ++i)
            draw_item(y++, sw, i);
        if (y <= bottom)
            y++;
        int nreq = static_cast<int>(items_.size()) - n_chats_;
        attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
        if (y <= bottom)
            mvaddstr(y++, 0, clip(" REQUESTS (" + std::to_string(nreq) + ")", sw).c_str());
        attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
        if (nreq == 0 && y <= bottom) {
            attron(A_DIM);
            mvaddstr(y++, 0, clip("  none", sw).c_str());
            attroff(A_DIM);
        }
        for (int i = n_chats_; i < static_cast<int>(items_.size()) && y <= bottom; ++i)
            draw_item(y++, sw, i);

        // Conversation pane.
        draw_conversation(top, bottom, sw + 2, cols - (sw + 2));

        // Input line.
        std::string prompt = "> " + input_;
        attron(COLOR_PAIR(CP_YOU));
        mvaddstr(rows - 2, 0, clip(prompt, cols).c_str());
        attroff(COLOR_PAIR(CP_YOU));

        // Help line.
        attron(COLOR_PAIR(CP_DIM) | A_DIM);
        mvhline(rows - 1, 0, ' ', cols);
        mvaddstr(rows - 1, 0,
                 clip(" ↑/↓ select  Enter send  a/r accept/reject  /add /id /quit", cols).c_str());
        attroff(COLOR_PAIR(CP_DIM) | A_DIM);

        move(rows - 2, std::min(cols - 1, 2 + static_cast<int>(input_.size())));
        refresh();
    }

    void draw_item(int y, int sw, int i) {
        const Item& it = items_[static_cast<std::size_t>(i)];
        bool sel = (i == selected_);
        if (sel)
            attron(COLOR_PAIR(CP_SELECTED));
        if (sel)
            mvhline(y, 0, ' ', sw);
        std::string label = " " + it.name;
        mvaddstr(y, 0, clip(label, sw).c_str());
        if (!it.subtitle.empty()) {
            std::string sub = it.subtitle;
            attron(A_DIM);
            mvaddstr(y, std::min(sw - 1, 1 + static_cast<int>(it.name.size()) + 2),
                     clip(sub, sw - (static_cast<int>(it.name.size()) + 3)).c_str());
            attroff(A_DIM);
        }
        if (sel)
            attroff(COLOR_PAIR(CP_SELECTED));
    }

    void draw_conversation(int top, int bottom, int x, int w) {
        if (w < 4)
            return;
        // Wrap lines.
        std::vector<std::pair<std::string, int>> wrapped; // text, color pair
        for (const auto& l : conv_) {
            std::string head = l.time.empty() ? "" : (l.time + "  ");
            std::string prefix = head + (l.who.empty() ? "" : (l.who + ": "));
            std::string full = prefix + l.text;
            int cp = l.outgoing ? CP_YOU : CP_DIM;
            // simple wrap
            for (std::size_t p = 0; p < full.size() || p == 0;) {
                std::string chunk = full.substr(p, static_cast<std::size_t>(w - 1));
                wrapped.emplace_back(chunk, cp);
                if (chunk.size() < static_cast<std::size_t>(w - 1) ||
                    p + chunk.size() >= full.size())
                    break;
                p += chunk.size();
            }
            if (full.empty())
                wrapped.emplace_back("", cp);
        }
        int avail = bottom - top + 1;
        int start = std::max(0, static_cast<int>(wrapped.size()) - avail - scroll_);
        int y = top;
        for (int i = start; i < static_cast<int>(wrapped.size()) && y <= bottom; ++i, ++y) {
            attron(COLOR_PAIR(wrapped[static_cast<std::size_t>(i)].second));
            mvaddstr(y, x, wrapped[static_cast<std::size_t>(i)].first.c_str());
            attroff(COLOR_PAIR(wrapped[static_cast<std::size_t>(i)].second));
        }
    }

    const Item* selected_item() const {
        if (selected_ >= 0 && selected_ < static_cast<int>(items_.size()))
            return &items_[static_cast<std::size_t>(selected_)];
        return nullptr;
    }

    void on_key(int ch) {
        switch (ch) {
        case KEY_UP:
            if (selected_ > 0)
                selected_--;
            scroll_ = 0;
            return;
        case KEY_DOWN:
            selected_++;
            scroll_ = 0;
            return;
        case KEY_PPAGE:
            scroll_ += 3;
            return;
        case KEY_NPAGE:
            scroll_ = std::max(0, scroll_ - 3);
            return;
        case KEY_RESIZE:
            return;
        case KEY_BACKSPACE:
        case 127:
        case 8:
            if (!input_.empty())
                input_.pop_back();
            return;
        case '\n':
        case KEY_ENTER:
            handle_enter();
            return;
        default:
            break;
        }
        // 'a'/'r' quick accept/reject when a request is selected and input is empty.
        if (input_.empty() && (ch == 'a' || ch == 'r')) {
            if (const Item* it = selected_item(); it && it->is_request) {
                submit(std::string(ch == 'a' ? "/accept " : "/reject ") +
                       openmesh::core::to_hex(it->pk));
                return;
            }
        }
        if (ch >= 32 && ch < 256)
            input_.push_back(static_cast<char>(ch));
    }

    void handle_enter() {
        std::string line = input_;
        input_.clear();
        if (line.empty())
            return;
        if (line[0] != '/') {
            const Item* it = selected_item();
            if (it && !it->is_request) {
                submit("/send " + openmesh::core::to_hex(it->pk) + " " + line);
            } else {
                flash_ = "select a chat to message";
            }
            return;
        }
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd == "/accept" || cmd == "/reject") {
            const Item* it = selected_item();
            if (it && it->is_request)
                submit(cmd + " " + openmesh::core::to_hex(it->pk));
            else
                flash_ = "select a request first";
            return;
        }
        submit(line);
    }

    Identity self_;
    std::optional<Endpoint> relay_;
    std::string name_, store_path_, store_pass_;
    Engine engine_;
    std::unique_ptr<SignalingClient> sig_;

    std::mutex mtx_;  // guards engine_ + snapshot building
    std::mutex qmtx_; // guards queue_
    std::deque<std::string> queue_;
    std::atomic<bool> running_{true};

    // UI-thread state.
    std::vector<Item> items_;
    int n_chats_ = 0;
    int selected_ = 0;
    int scroll_ = 0;
    std::string input_;
    std::vector<Line> conv_;
    std::string status_;
    std::string flash_;
};

} // namespace

int run_tui(Identity self, std::optional<Endpoint> server, std::optional<Endpoint> relay,
            std::string display_name, std::string store_path, std::string store_pass) {
    Tui tui(std::move(self), server, relay, std::move(display_name), std::move(store_path),
            std::move(store_pass));
    return tui.run();
}

} // namespace omchat
