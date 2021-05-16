/**
 * @brief Main
 * @anchor Ho 229
 * @date 2021/4/10
 */

#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "videoplayer.h"
#include "keyboardcontrollor.h"

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

    KeyboardControllor qmlKey;
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    engine.rootContext()->setContextProperty("qmlKey", &qmlKey);

    const QList<QObject *> objList = engine.rootObjects();
    objList.first()->installEventFilter(&qmlKey);

    QObject::connect(&qmlKey, SIGNAL(play()), objList.first(),
                     SLOT(onPlay()));
    QObject::connect(&qmlKey, SIGNAL(back()), objList.first(),
                     SLOT(onBack()));
    QObject::connect(&qmlKey, SIGNAL(goahead()), objList.first(),
                     SLOT(onGoahead()));

    return app.exec();
}
