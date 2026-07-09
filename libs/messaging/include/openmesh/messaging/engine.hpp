#pragma once

#include "openmesh/messaging/session.hpp"
#include "openmesh/net/endpoint.hpp"
#include "openmesh/net/udp_socket.hpp"
#include "openmesh/storage/store.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace openmesh::messaging {

// A decrypted message as seen by the application layer (SRS FR-4).
struct Message {
    Bytes peer;      // remote identity (Ed25519 public key)
    Bytes plaintext; // decrypted content (only ever in memory / local storage)
    bool outgoing = false;
};

// The UI-independent Messaging Engine (SRS §10). Ties the crypto sessions to the
// UDP transport and the local contact database, enforcing the contact-request
// flow (SRS FR-2, FR-3, §7): unknown users may send exactly one contact request;
// only accepted contacts can exchange messages.
//
// Two ways a peer becomes a trusted contact:
//   * add_peer()               — out-of-band: keys exchanged already, trust it now.
//   * send_contact_request() + the peer accepts  (or we accept theirs).
//
// No Qt/UI types. Synchronous poll() for v1.
class Engine {
public:
    using MessageHandler = std::function<void(const Message&)>;
    using ContactRequestHandler = std::function<void(const Bytes& peer, const Bytes& greeting)>;
    using ContactResponseHandler = std::function<void(const Bytes& peer, bool accepted)>;

    Engine(Bytes local_public, Bytes local_secret);

    bool bind(const net::Endpoint& local);
    [[nodiscard]] std::optional<net::Endpoint> local_endpoint() const;
    [[nodiscard]] net::UdpSocket& transport() { return socket_; }

    // Announce our presence to a relay server (SRS FR-8): sends a source-tagged
    // keepalive so the relay learns the endpoint to reach us at. Call on startup
    // and periodically. To route messages via the relay, use the relay's address
    // as the peer endpoint in add_peer()/send_contact_request(); the relay
    // forwards by destination public key.
    bool announce_to(const net::Endpoint& relay);

    // Make the contact database durable and encrypted at rest (SRS FR-9): loads
    // any existing contacts and auto-persists subsequent changes. Returns false
    // on a wrong passphrase / corrupt file. Runtime state (sessions, peer
    // addresses, in-flight requests) is not persisted and is re-established after
    // a restart via add_peer()/discovery.
    bool open_store(const std::string& path, const std::string& passphrase) {
        return contacts_.open(path, passphrase);
    }

    // Trust a peer directly (keys exchanged out of band): establishes the session
    // and marks the contact Accepted. Returns false if key agreement fails.
    bool add_peer(const Bytes& remote_public, const net::Endpoint& endpoint);
    [[nodiscard]] bool knows_peer(const Bytes& remote_public) const;
    [[nodiscard]] std::optional<storage::TrustStatus> trust_of(const Bytes& remote_public) const;

    // --- Contact-request flow (SRS FR-2) ------------------------------------
    // Send a one-shot contact request to a peer (optionally with a greeting).
    // Marks the contact Pending. Returns false if already an accepted contact.
    bool send_contact_request(const Bytes& remote_public, const net::Endpoint& endpoint,
                              const Bytes& greeting = {});
    // Accept / reject a pending *incoming* request; notifies the requester.
    bool accept_contact(const Bytes& remote_public);
    bool reject_contact(const Bytes& remote_public);

    // Encrypt and send a message. Fails unless the peer is an accepted contact.
    bool send(const Bytes& remote_public, const Bytes& plaintext);

    // Receive at most one datagram (waiting up to timeout_ms) and dispatch it:
    // deliver a message, surface a contact request/response, or drop it (e.g. a
    // message from a non-accepted peer). Returns true iff something was handled.
    bool poll(int timeout_ms);

    void on_message(MessageHandler handler) { message_handler_ = std::move(handler); }
    void on_contact_request(ContactRequestHandler handler) {
        request_handler_ = std::move(handler);
    }
    void on_contact_response(ContactResponseHandler handler) {
        response_handler_ = std::move(handler);
    }

private:
    Session* ensure_session(const Bytes& remote_public);
    bool send_packet(const std::string& key, const protocol::Packet& packet);

    void handle_message(const Bytes& peer, Bytes plaintext);
    void handle_contact_request(const Bytes& peer, Bytes greeting, const net::Endpoint& from);
    void handle_contact_response(const Bytes& peer, const Bytes& decision);

    Bytes local_public_;
    Bytes local_secret_;
    net::UdpSocket socket_;
    storage::Store contacts_;

    MessageHandler message_handler_;
    ContactRequestHandler request_handler_;
    ContactResponseHandler response_handler_;

    // Keyed by hex(remote public key).
    std::unordered_map<std::string, Session> sessions_;
    std::unordered_map<std::string, net::Endpoint> endpoints_;
    std::unordered_set<std::string> incoming_requests_; // awaiting our decision
    std::unordered_set<std::string> outgoing_requests_; // awaiting their response
};

} // namespace openmesh::messaging
