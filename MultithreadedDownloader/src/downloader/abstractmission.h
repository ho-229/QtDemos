﻿#ifndef ABSTRACTMISSION_H
#define ABSTRACTMISSION_H

#include <QUrl>
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

    virtual ~AbstractMission() = default;

    void setUrl(QUrl url)
    {
        m_url = url;
        if(!m_url.isValid())
            qWarning("AbstractMission : url is not valid.");
    }

    bool isFinished(){ return m_isFinished; }

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
        if(isFinished)  emit finished();
    }

    QUrl m_url;
    State m_state = Stopped;
    bool m_isFinished = false;

signals:
    void finished();
    void stateChanged(State state);
};

#endif // ABSTRACTMISSION_H