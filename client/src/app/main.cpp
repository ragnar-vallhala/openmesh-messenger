#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

// Desktop & Android entry point (SRS §1.2, §10). Boots the QML engine and loads
// the root component. The AppController bridge type is auto-registered via
// QML_ELEMENT and instantiated from QML.
int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("OpenMesh Messenger");
    app.setOrganizationName("OpenMesh");

    QQmlApplicationEngine engine;
    const QUrl rootUrl(QStringLiteral("qrc:/OpenMesh/qml/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [rootUrl](QObject* obj, const QUrl& objUrl) {
            if (obj == nullptr && objUrl == rootUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(rootUrl);

    return app.exec();
}
