/**
 * @brief AbstractMission
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef ABSTRACTMISSION_H
#define ABSTRACTMISSION_H

#include <QUrl>
#include <QDebug>
#include <QObject>

class AbstractMission : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        Running,
        Paused,
        Stopped
    };

    virtual ~AbstractMission() Q_DECL_OVERRIDE = default;

    template <typename T>
    void setUrl(const T& url)
    {
        m_url = url;
        if(!m_url.isValid())
            qWarning("AbstractMission : url is not valid.");
    }

    QUrl url() const { return m_url; }

    bool isFinished() const { return m_isFinished; }

    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void stop()  = 0;

    State state() const { return m_state; }

protected:
    explicit AbstractMission(QObject* parent = nullptr) : QObject(parent) {}

    inline void updateState(const State state)
    {
        m_state = state;
        emit stateChanged(state);
    }

    inline void setFinished(const bool isFinished = true)
    {
        m_isFinished = isFinished;

        if(isFinished)
            emit finished();
    }

    QUrl m_url;
    State m_state = Stopped;
    bool m_isFinished = false;

signals:
    void finished();
    void stateChanged(const AbstractMission::State state);
};

#endif // ABSTRACTMISSION_H
