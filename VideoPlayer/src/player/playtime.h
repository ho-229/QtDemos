/**
 * @brief Play Time
 * @anchor Ho 229
 * @date 2021/5/4
 */

#ifndef PLAYTIME_H
#define PLAYTIME_H

#include <QDateTime>

class PLayTime
{
public:
    PLayTime() {}

    void restart() { m_startMSecs = QDateTime::currentMSecsSinceEpoch(); }

    qint64 elapsed() const { return QDateTime::currentMSecsSinceEpoch() - m_startMSecs; }

    void synchronize(qint64 msecs) { m_startMSecs += (this->elapsed() - msecs); }

private:
    qint64 m_startMSecs = 0;
};

#endif // PLAYTIME_H
