/**
 * @brief Main
 * @anchor Ho 229
 * @date 2021/4/10
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "videoplayer.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

#ifdef Q_OS_WIN
    app.setFont(QFont("Microsoft YaHei", 9));
#endif

    qmlRegisterType<VideoPlayer>("com.multimedia.videoplayer", 1, 0, "VideoPlayer");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
