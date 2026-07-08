# Software Requirements Specification (SRS)

## Project Name

**OpenMesh Messenger (Working Title)**

---

# 1. Introduction

## 1.1 Purpose

OpenMesh Messenger is a decentralized, end-to-end encrypted messaging platform designed to operate without a central authority controlling user accounts, conversations, or identities.

The system allows anyone to communicate securely while retaining complete ownership of their identity and data.

---

## 1.2 Goals

* No central organization owns the network.
* Users own their cryptographic identities.
* Messages are end-to-end encrypted.
* Anyone can run signaling or relay infrastructure.
* Communication continues even if some infrastructure disappears.
* Support Desktop and Android from a single Qt codebase.

---

## 1.3 Scope

Version 1 focuses on:

* One-to-one messaging
* Contact requests
* End-to-end encryption
* Peer discovery
* Volunteer signaling servers
* Relay support when direct communication is unavailable

Future versions may include:

* Group chats
* Voice/video calls
* File transfer
* Distributed peer discovery (DHT)
* Offline encrypted message storage
* Mesh networking

---

# 2. System Overview

The system consists of:

* Desktop Client
* Android Client
* Signaling Server
* Relay Server (optional)

No component stores plaintext messages.

---

# 3. Design Principles

* Decentralized
* Privacy First
* Zero Trust
* Cryptography before Authentication
* Local Data Ownership
* Open Protocol
* Modular Architecture

---

# 4. User Identity

Every user owns:

* Public Key
* Private Key

The public key (or its fingerprint) becomes the user's permanent identity.

No usernames, emails, or phone numbers are required.

---

# 5. Functional Requirements

## FR-1 Identity

The client shall:

* Generate cryptographic key pairs.
* Store private keys securely.
* Export/import identities.
* Display a shareable public identity.

---

## FR-2 Contact Requests

Unknown users may send exactly one contact request.

The recipient can:

* Accept
* Reject
* Ignore

Only accepted users can exchange regular messages.

---

## FR-3 Contacts

The client shall maintain a local trusted contact database.

Each contact contains:

* Public Key
* Nickname
* Trust Status
* Last Seen
* Session Information

---

## FR-4 Messaging

The system shall support:

* Text messages
* Message timestamps
* Delivery status
* Read receipts (optional)
* Message ordering

Messages shall remain encrypted during transport.

---

## FR-5 End-to-End Encryption

The system shall:

* Establish secure sessions.
* Encrypt every message.
* Authenticate every packet.
* Reject tampered packets.

Neither signaling nor relay servers shall be able to decrypt messages.

---

## FR-6 Signaling

Signaling servers shall:

* Register active peers.
* Exchange connection information.
* Forget inactive peers automatically.
* Never possess private keys.
* Never decrypt traffic.

Anyone shall be able to host a signaling server.

---

## FR-7 Peer Connection

Clients shall attempt:

1. Direct connection.
2. NAT traversal.
3. Relay fallback.

---

## FR-8 Relay

Relay nodes shall:

* Forward encrypted packets.
* Store no plaintext.
* Remain protocol-neutral.
* Be independently deployable.

---

## FR-9 Local Storage

Messages shall be stored locally.

Local storage includes:

* Contacts
* Conversations
* Identity
* Preferences

No cloud account is required.

---

## FR-10 Multi-Device (Future)

Users may authorize multiple devices under the same identity.

---

# 6. Non-Functional Requirements

## Performance

* Connection establishment < 5 seconds under normal conditions.
* Typical message latency < 500 ms on stable networks.
* Idle memory usage should remain low.
* Battery usage optimized on Android.

---

## Reliability

* Automatic reconnection.
* Network interruption recovery.
* Message retransmission when necessary.

---

## Security

* End-to-end encryption.
* Perfect Forward Secrecy (future).
* Secure key storage.
* Replay protection.
* Packet authentication.

---

## Scalability

The protocol shall support:

* Millions of identities.
* Thousands of independent signaling servers.
* Thousands of relay nodes.

No central bottleneck shall exist.

---

# 7. Spam Prevention

Unknown users may only send one contact request.

Until accepted:

* Additional messages are rejected.
* No chat session exists.

Future enhancements:

* Proof-of-Work
* Reputation
* Rate limiting
* Block lists

---

# 8. Networking

Transport:

* UDP (initial)
* QUIC (future)

Protocol layers:

Application Protocol

↓

Encryption Layer

↓

Transport Layer

↓

Network Layer

---

# 9. Packet Types

* HELLO
* REGISTER
* DISCOVER
* CONNECT
* CONTACT_REQUEST
* CONTACT_RESPONSE
* MESSAGE
* ACK
* HEARTBEAT
* DISCONNECT

---

# 10. Client Architecture

UI Layer (Qt/QML)

↓

Application Layer

↓

Messaging Engine

↓

Crypto Engine

↓

Networking Engine

↓

Socket Layer

The Messaging Engine shall remain independent of the UI.

---

# 11. Signaling Server Architecture

Responsibilities:

* Maintain active peer registry.
* Provide peer lookup.
* Exchange ICE/connection information.
* Remove expired entries.

The server shall never:

* Store conversations.
* Read messages.
* Authenticate users beyond proof of key ownership.

---

# 12. Relay Server

Responsibilities:

* Forward encrypted packets.
* Maintain temporary routing state.
* Support multiple simultaneous clients.

The relay shall remain content-agnostic.

---

# 13. Data Ownership

Users own:

* Identity
* Contacts
* Messages
* Encryption keys

Servers own nothing except temporary routing information.

---

# 14. Future Roadmap

Version 1

* Identity
* Contact requests
* Direct messaging
* Signaling
* Relay

Version 2

* Group messaging
* Attachments
* Push notifications
* Multiple devices

Version 3

* Distributed Hash Table (DHT)
* Fully decentralized discovery
* Offline encrypted message storage
* Voice/video calls

---

# 15. Success Criteria

The project shall be considered successful when:

* Two users can discover each other through any public signaling server.
* They establish an encrypted session.
* They exchange messages without any central authority storing conversations.
* Multiple independent signaling servers can coexist.
* Users retain complete ownership of their identities and message history.

