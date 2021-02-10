/**
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "mainwidget.h"

#include <QLocale>
#include <QTranslator>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    a.setFont(QFont("Microsoft YaHei",9));
#endif

    QTranslator tr_CN;
    if(QLocale::system().language() == QLocale::Chinese)
    {
        tr_CN.load(":/translations/MultithreadedDownloader_zh_CN.qm");
        a.installTranslator(&tr_CN);
    }

    a.setApplicationName("Multithreaded Downloader");

    QCommandLineOption threadNumOpt({"t", "thread-number"},
                                    "Set the thread number for download.",
                                    "number",
                                    "0");

    QCommandLineOption urlOpt({"u", "url"},
                              "The URL to download.",
                              "URL");

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Multithreaded Downloader Example.");
    parser.addHelpOption();
    parser.addOption(urlOpt);
    parser.addOption(threadNumOpt);
    parser.process(a);

    MainWidget w;
    w.initThreadNumber(parser.value(threadNumOpt).toInt());
    w.show();
    w.initDownloadUrl(parser.value(urlOpt));
    return a.exec();
}
