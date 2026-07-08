# openmesh::storage

Local, device-owned persistence (SRS FR-9, §13). No cloud account is required;
the user owns all of their data.

Stores:

- **Identity** — the local key pair (private key kept in secure storage).
- **Contacts** — the trusted contact database: public key, nickname, trust
  status, last-seen, session info (SRS FR-3).
- **Conversations** — message history with timestamps, delivery status and
  ordering (SRS FR-4).
- **Preferences** — client settings.

Design notes:
- At-rest encryption of the local store is expected (the private key and message
  history must be protected on-device).
- Backend is TBD (SQLite is a strong default). Kept behind this interface so the
  choice can change without touching the Messaging Engine or UI.
