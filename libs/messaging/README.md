# openmesh::messaging

The **Messaging Engine** (SRS §10). Orchestrates crypto, networking and storage
to deliver the application's behaviour — and is deliberately **independent of the
UI** so it can be driven by the desktop app, the Android app, or a headless test
harness identically (SRS §10: "The Messaging Engine shall remain independent of
the UI").

Responsibilities:

- Drive contact requests: enforce the one-request rule and accept/reject/ignore
  handling (SRS FR-2).
- Send/receive text messages with timestamps, delivery status, ordering and
  optional read receipts (SRS FR-4).
- Establish encrypted sessions via `crypto`, ship packets via `net`, and persist
  everything via `storage`.
- Expose an event/callback surface the UI layer subscribes to (no Qt types leak
  in here).

This is the top of the shared-library stack: it depends on `core`, `protocol`,
`crypto`, `net` and `storage`, and is consumed by `client/`.
