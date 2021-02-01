/**
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "mainwidget.h"

#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <QApplication>

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

    MainWidget w;
    w.show();
    return a.exec();
}
