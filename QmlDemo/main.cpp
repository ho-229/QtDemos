#include <QFont>
#include <QTranslator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "clickwaveeffect.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

#ifdef Q_OS_WIN
    QGuiApplication::setFont(QFont("Microsoft YaHei", 12));
#endif

    QTranslator tr_CN;
    if(QLocale::system().language() == QLocale::Chinese)
    {
        tr_CN.load(":/translate/QmlDemo_zh_CN.qm");
        app.installTranslator(&tr_CN);
    }

    qmlRegisterType<ClickWaveEffect>("com.MyItems.Effect", 1, 0, "ClickWaveEffect");

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
