# OpenMesh Client

The single Qt/QML application targeting **Desktop and Android from one codebase**
(SRS §1.2). It is a thin shell over the shared libraries — all real logic lives
in `libs/`, especially the UI-independent Messaging Engine (`openmesh::messaging`,
SRS §10).

## Layout

```
client/
├── src/
│   ├── app/        # main(), QGuiApplication + QML engine bootstrap
│   ├── bridge/     # C++ ↔ QML bridge objects (QObject-derived controllers)
│   ├── models/     # Qt item models exposing contacts/conversations to QML
│   └── ui/         # C++-side UI helpers (theming, formatters)
├── qml/
│   ├── components/ # Reusable QML controls
│   └── screens/    # Full screens (Contacts, Chat, Identity, ...)
├── resources/      # Icons, fonts, .qrc bundles
└── platform/
    ├── desktop/    # Desktop-specific packaging/glue
    └── android/    # Android manifest, Gradle, JNI glue, key storage
```

## Architecture (SRS §10)

```
QML UI  ->  bridge/models (C++)  ->  openmesh::messaging  ->  crypto / net / storage
```

The bridge layer contains the only Qt-aware code that touches the engine; the
engine itself never depends on Qt, so it can be unit-tested headlessly.

## Build

Requires Qt 6.4+. Built as part of the top-level CMake project (`OPENMESH_BUILD_CLIENT=ON`).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target openmesh-client -j
```

### Android

Android builds use the same sources via Qt for Android + the Qt Gradle plugin;
platform-specific manifest and secure key storage live in `platform/android/`
(to be filled in).
