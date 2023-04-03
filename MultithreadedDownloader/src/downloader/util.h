/**
 * @brief Until
 * @anchor NiceBlueChai
 * @date 2020/07/16
 */

#ifndef UTIL_H
#define UTIL_H

#include <QString>

namespace Until {

    /**
     * @brief readableFileSize
     * @param value
     * @param precision
     * @return readable size
     */
    QString readableFileSize(const qint64 value, int precision = 2)
    {
        qint64 kbSize = value / 1024;
        if (kbSize > 1024)
        {
            qreal mbRet = static_cast<qreal>(kbSize) / 1024.0;

            if (mbRet - 1024.0 > 0.000001)
            {
                qreal gbRet = mbRet / 1024.0;
                return QString::number(gbRet, 'f', precision) + "GB";
            }
            else
                return QString::number(mbRet, 'f', precision) + "MB";
        }
        else
            return QString::number(kbSize) + "KB";
    }
}

#endif // UTIL_H
