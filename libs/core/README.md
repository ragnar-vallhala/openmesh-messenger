# openmesh::core

Foundation utilities shared by every other library and component. Deliberately
tiny and dependency-free (no Qt, no networking, no crypto) so it can be linked
anywhere — client, servers and tools alike.

Intended contents:

- **version** — build/version constants for the whole project.
- **logging** — a minimal severity-tagged logger (`OM_LOG_*`), later pluggable.
- **ids** — fingerprint / message-id value types (TODO).
- **errors** — a common `Result`/error-code vocabulary (TODO).
- **config** — parsing of server/client configuration (TODO).

Rule of thumb: if two unrelated components both need it and it has no external
dependency, it belongs here.
