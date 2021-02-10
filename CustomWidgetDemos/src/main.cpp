/**
 * @brief Demo
 * @anchor Ho229
 * @date 2020/12/12
 */

#include "mainwidget.h"

#include <QTextCodec>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    a.setFont(QFont("Microsoft YaHei",9));
#endif

    QTextCodec::setCodecForLocale(                  // Set codec to UTF-8
                QTextCodec::codecForName("UTF-8"));

    MainWidget w;
    w.show();
    return a.exec();
}
