# The single Qt/QML application shared by Desktop and Android (SRS §1.2, §10).
#
# The UI (QML) sits on top of the C++ application layer, which drives the
# UI-independent Messaging Engine (openmesh::messaging).

find_package(Qt6 6.4 QUIET COMPONENTS Core Quick Qml)

if(NOT Qt6_FOUND)
    message(WARNING
        "Qt6 (Core/Quick/Qml) not found — skipping the client build. "
        "Install Qt 6 or configure with -DOPENMESH_BUILD_CLIENT=OFF to silence this.")
    return()
endif()

qt_standard_project_setup(REQUIRES 6.4)

qt_add_executable(openmesh-client
    src/app/main.cpp
    src/bridge/app_controller.cpp
)

qt_add_qml_module(openmesh-client
    URI OpenMesh
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/screens/ContactsScreen.qml
        qml/screens/ChatScreen.qml
)

target_include_directories(openmesh-client PRIVATE src src/bridge)
target_link_libraries(openmesh-client PRIVATE
    Qt6::Quick
    openmesh::messaging
    openmesh_warnings)

set_target_properties(openmesh-client PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON)
