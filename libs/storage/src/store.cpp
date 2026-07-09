#include "openmesh/storage/store.hpp"

#include "blob.hpp"
#include "openmesh/crypto/kdf.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <utility>

namespace openmesh::storage {
namespace {

constexpr std::array<char, 4> kMagic = {'O', 'M', 'D', 'B'};

constexpr std::uint8_t kPayloadVersion = 2; // v2 adds messages + requests

void encode_contacts(detail::Bytes& out, const std::vector<Contact>& contacts) {
    detail::put_u32(out, static_cast<std::uint32_t>(contacts.size()));
    for (const auto& c : contacts) {
        detail::put_bytes(out, c.public_key);
        detail::put_str(out, c.nickname);
        detail::put_u8(out, static_cast<std::uint8_t>(c.trust));
        detail::put_u64(out, static_cast<std::uint64_t>(c.last_seen));
    }
}

bool decode_contacts(detail::Reader& r, std::vector<Contact>& out) {
    std::uint32_t count = 0;
    if (!r.u32(count)) {
        return false;
    }
    out.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        Contact c;
        std::uint8_t trust = 0;
        std::uint64_t last_seen = 0;
        if (!r.bytes(c.public_key) || !r.str(c.nickname) || !r.u8(trust) || !r.u64(last_seen)) {
            return false;
        }
        c.trust = static_cast<TrustStatus>(trust);
        c.last_seen = static_cast<std::int64_t>(last_seen);
        out.push_back(std::move(c));
    }
    return true;
}

detail::Bytes encode(const std::vector<Contact>& contacts, const std::vector<Message>& messages,
                     const std::vector<Request>& requests) {
    detail::Bytes out;
    detail::put_u8(out, kPayloadVersion);
    encode_contacts(out, contacts);

    detail::put_u32(out, static_cast<std::uint32_t>(messages.size()));
    for (const auto& m : messages) {
        detail::put_bytes(out, m.peer);
        detail::put_str(out, m.text);
        detail::put_u8(out, m.outgoing ? 1 : 0);
        detail::put_u64(out, static_cast<std::uint64_t>(m.timestamp));
    }
    detail::put_u32(out, static_cast<std::uint32_t>(requests.size()));
    for (const auto& q : requests) {
        detail::put_bytes(out, q.peer);
        detail::put_str(out, q.greeting);
        detail::put_u64(out, static_cast<std::uint64_t>(q.timestamp));
    }
    return out;
}

bool decode(const detail::Bytes& blob, std::vector<Contact>& contacts,
            std::vector<Message>& messages, std::vector<Request>& requests) {
    detail::Reader r(blob);
    // Legacy (v1) blobs are contacts-only and start with the contact count; v2
    // starts with the version byte 0x02.
    if (!blob.empty() && blob[0] == kPayloadVersion) {
        std::uint8_t version = 0;
        r.u8(version);
        if (!decode_contacts(r, contacts)) {
            return false;
        }
        std::uint32_t mcount = 0;
        if (!r.u32(mcount)) {
            return false;
        }
        for (std::uint32_t i = 0; i < mcount; ++i) {
            Message m;
            std::uint8_t outgoing = 0;
            std::uint64_t ts = 0;
            if (!r.bytes(m.peer) || !r.str(m.text) || !r.u8(outgoing) || !r.u64(ts)) {
                return false;
            }
            m.outgoing = outgoing != 0;
            m.timestamp = static_cast<std::int64_t>(ts);
            messages.push_back(std::move(m));
        }
        std::uint32_t qcount = 0;
        if (!r.u32(qcount)) {
            return false;
        }
        for (std::uint32_t i = 0; i < qcount; ++i) {
            Request q;
            std::uint64_t ts = 0;
            if (!r.bytes(q.peer) || !r.str(q.greeting) || !r.u64(ts)) {
                return false;
            }
            q.timestamp = static_cast<std::int64_t>(ts);
            requests.push_back(std::move(q));
        }
        return r.done();
    }
    // Legacy contacts-only.
    return decode_contacts(r, contacts) && r.done();
}

} // namespace

bool Store::open(const std::string& path, const std::string& passphrase) {
    if (std::filesystem::exists(path)) {
        auto salt = detail::read_salt(path, kMagic);
        if (!salt) {
            return false;
        }
        auto key = crypto::derive_key(passphrase, *salt);
        if (key.empty()) {
            return false;
        }
        auto plaintext = detail::read_blob(path, kMagic, key);
        if (!plaintext) {
            return false; // wrong passphrase or tampered
        }
        std::vector<Contact> c;
        std::vector<Message> m;
        std::vector<Request> q;
        if (!decode(*plaintext, c, m, q)) {
            return false;
        }
        contacts_ = std::move(c);
        messages_ = std::move(m);
        requests_ = std::move(q);
        salt_ = std::move(*salt);
        key_ = std::move(key);
        path_ = path;
        persistent_ = true;
        return true;
    }

    // New store: fresh salt + derived key, then create the file.
    auto salt = crypto::generate_salt();
    auto key = crypto::derive_key(passphrase, salt);
    if (key.empty()) {
        return false;
    }
    salt_ = std::move(salt);
    key_ = std::move(key);
    path_ = path;
    persistent_ = true;
    return save();
}

bool Store::save() const {
    if (!persistent_) {
        return false;
    }
    return detail::write_blob(path_, kMagic, salt_, key_, encode(contacts_, messages_, requests_));
}

void Store::add_contact(const Contact& contact) {
    contacts_.push_back(contact);
    if (persistent_) {
        save();
    }
}

void Store::upsert(const Contact& contact) {
    for (auto& c : contacts_) {
        if (c.public_key == contact.public_key) {
            c = contact;
            if (persistent_) {
                save();
            }
            return;
        }
    }
    contacts_.push_back(contact);
    if (persistent_) {
        save();
    }
}

std::vector<Contact> Store::contacts() const {
    return contacts_;
}

std::optional<Contact> Store::find(const std::vector<std::uint8_t>& public_key) const {
    for (const auto& c : contacts_) {
        if (c.public_key == public_key) {
            return c;
        }
    }
    return std::nullopt;
}

std::optional<TrustStatus> Store::trust_of(const std::vector<std::uint8_t>& public_key) const {
    for (const auto& c : contacts_) {
        if (c.public_key == public_key) {
            return c.trust;
        }
    }
    return std::nullopt;
}

void Store::append_message(const Message& message) {
    messages_.push_back(message);
    if (persistent_) {
        save();
    }
}

std::vector<Message> Store::messages() const {
    return messages_;
}

std::vector<Message> Store::conversation(const std::vector<std::uint8_t>& peer) const {
    std::vector<Message> out;
    for (const auto& m : messages_) {
        if (m.peer == peer) {
            out.push_back(m);
        }
    }
    return out;
}

void Store::add_request(const Request& request) {
    for (const auto& q : requests_) {
        if (q.peer == request.peer) {
            return; // already pending
        }
    }
    requests_.push_back(request);
    if (persistent_) {
        save();
    }
}

void Store::remove_request(const std::vector<std::uint8_t>& peer) {
    const auto before = requests_.size();
    requests_.erase(std::remove_if(requests_.begin(), requests_.end(),
                                   [&](const Request& q) { return q.peer == peer; }),
                    requests_.end());
    if (persistent_ && requests_.size() != before) {
        save();
    }
}

std::vector<Request> Store::requests() const {
    return requests_;
}

} // namespace openmesh::storage
