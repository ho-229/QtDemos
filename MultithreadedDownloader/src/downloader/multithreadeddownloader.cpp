/**
 * @brief MultithreadedDownloader
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "multithreadeddownloader.h"

#include <QEventLoop>
#include <QTimerEvent>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

MultithreadedDownloader::MultithreadedDownloader(QObject *parent)
    : AbstractMission(parent),
      m_manager(new QNetworkAccessManager(this)),
      m_writer(new MultithreadedDownloaderWriter(this)),
      m_threadNumber(QThread::idealThreadCount())
{

}

MultithreadedDownloader::~MultithreadedDownloader()
{
    if(m_state == Running)
        this->MultithreadedDownloader::stop();

    if(m_writer->isRunning())
        m_writer->terminate();
}

bool MultithreadedDownloader::getFileInfo()
{
    if(!m_url.isValid() || m_state != Stopped)
        return false;

    QNetworkRequest request(m_url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply *reply = m_manager->head(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    bool ok = false;
    if(reply->hasRawHeader("Content-Length"))
        m_writer->setSize(reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(&ok));

    if(reply->hasRawHeader("Content-Disposition"))
        m_writer->setFileName(reply->header(QNetworkRequest::ContentDispositionHeader)
                              .toString().split("filename=").at(1));
    else
        m_writer->setFileName(reply->url().fileName());

    return ok && m_writer->size() > 0 && (!m_writer->fileName().isEmpty());
}

void MultithreadedDownloader::start()
{
    if(m_state == Running || m_writer->fileName().isEmpty() ||
            m_writer->size() <= 0 || m_url.isEmpty())
        return;

    if(m_state == Stopped)
    {
        if(!m_writer->openFile())
        {
            emit error(OpenFileFailed);
            return;
        }

        qint64 start, end;
        qint64 segmentSize = m_writer->size() / m_threadNumber;
        for(int i = 0; i < m_threadNumber; i++)
        {
            start = i * segmentSize;
            if(i != m_threadNumber - 1)
                end = start + segmentSize -1;
            else
                end = m_writer->size();               // Last mission

            m_missions.push_back(this->createMission(start, end));
        }

        this->setFinished(false);
    }
    else
    {
        const QList<DownloadMission *>& constlist = m_missions;
        for(DownloadMission *mission : constlist)
            if(!mission->isFinished())
                mission->start();
    }

    m_timerId = this->startTimer(400);
    this->updateState(Running);
}

void MultithreadedDownloader::pause()
{
    if(m_state == Running)
    {
        const QList<DownloadMission *>& constlist = m_missions;
        for(DownloadMission *misson : constlist)
            if(!misson->isFinished())
                misson->pause();

        this->killTimer(m_timerId);
        this->updateState(Paused);
    }
}

void MultithreadedDownloader::stop()
{
    if(m_state != Stopped)
    {
        this->killTimer(m_timerId);
        this->destoryMissions();

        if(m_writer->isRunning())           // Wait for the write finish
        {
            QEventLoop loop;
            QObject::connect(m_writer, &MultithreadedDownloaderWriter::finished, &loop,
                             &QEventLoop::quit);
            loop.exec();
        }

        m_writer->closeFile();
        m_finishedCount = 0;

        this->updateState(Stopped);
    }
}

void MultithreadedDownloader::errorHanding(QNetworkReply::NetworkError err)
{
    m_networkError = err;
    m_networkErrorString = qobject_cast<DownloadMission *>
            (this->sender())->replyErrorString();
    if(err == QNetworkReply::OperationCanceledError || err == QNetworkReply::NoError)
        return;

    this->stop();
    emit error(DownloadFailed);
}

void MultithreadedDownloader::on_finished()
{
    m_finishedCount++;

    qDebug() << "MultithreadedDownloader: finishedCount:" << m_finishedCount;

    if(m_finishedCount == m_threadNumber)
    {
        this->updateProgress();
        this->stop();

        emit finished();
    }
}

DownloadMission *MultithreadedDownloader::createMission(qint64 start, qint64 end)
{
    DownloadMission *mission = new DownloadMission(this);

    QObject::connect(mission, &DownloadMission::finished, this,
                     &MultithreadedDownloader::on_finished);
    QObject::connect(mission, &DownloadMission::replyError, this,
                     &MultithreadedDownloader::errorHanding);

    mission->setManager(m_manager);
    mission->setWriter(m_writer);
    mission->setRange(start, end);
    mission->setUrl(m_url);
    mission->start();

    return mission;
}

void MultithreadedDownloader::destoryMissions()
{
    const QList<DownloadMission *>& constlist = m_missions;
    for(DownloadMission *mission : constlist)
    {
        if(mission->state() != Stopped)
            mission->stop();
        mission->deleteLater();
    }
    m_missions.clear();
}

void MultithreadedDownloader::updateProgress()
{
    qint64 bytesReceived = 0;

    const QList<DownloadMission *>& constlist = m_missions;
    for(DownloadMission* mission : constlist)
        bytesReceived += mission->downloadedSize();

    emit downloadProgress(bytesReceived, m_writer->size());
}

void MultithreadedDownloader::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_timerId)
        this->updateProgress();
}
