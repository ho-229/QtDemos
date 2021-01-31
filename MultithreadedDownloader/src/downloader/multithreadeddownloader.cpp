﻿#include "multithreadeddownloader.h"

#include <QThread>
#include <QEventLoop>
#include <QBasicTimer>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

MultithreadedDownloader::MultithreadedDownloader(QObject *parent)
    : AbstractMission(parent),
      m_manager(new QNetworkAccessManager(this)),
      m_writer(new MultithreadedDownloaderWriter(this)),
      m_threadCount(QThread::idealThreadCount())
{

}

MultithreadedDownloader::~MultithreadedDownloader()
{
    if(m_state == Running)
        this->stop();
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
    loop.exec();

    bool ok = false;
    if(reply->hasRawHeader("Content-Length"))
        m_writer->setSize(reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(&ok));

    if(reply->hasRawHeader("Content-Disposition"))
        m_writer->setFileName(reply->header(QNetworkRequest::ContentDispositionHeader)
                              .toString().split("filename=")[1]);
    else
        m_writer->setFileName(reply->url().fileName());

    return ok && (!m_writer->fileName().isEmpty());
}

void MultithreadedDownloader::start()
{
    if(m_state == Running)
        return;

    if(m_state == Stopped)
    {
        if(!m_writer->openFile())
        {
            emit error(OpenFileFailed);
            return;
        }

        qint64 start, end;
        qint64 segmentSize = m_writer->size() / m_threadCount;
        for(int i = 0; i < m_threadCount; i++)
        {
            start = i * segmentSize;
            if(i != m_threadCount - 1)
                end = start + segmentSize -1;
            else
                end = m_writer->size();               // Last mission

            m_missions.push_back(this->createMission(start, end));
        }

        this->setFinished(false);
    }
    else
    {
        for(auto mission : m_missions)
            mission->start();
    }

    m_timerId = this->startTimer(400);
    this->updateState(Running);
}

void MultithreadedDownloader::pause()
{
    if(m_state == Running)
    {
        for(DownloadMission *misson : m_missions)
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
        m_writer->closeFile();

        this->destoryMissions();
        this->killTimer(m_timerId);
        this->updateState(Stopped);
    }
}

void MultithreadedDownloader::errorHanding(QNetworkReply::NetworkError err)
{
    m_networkError = err;
    if(err == QNetworkReply::OperationCanceledError || err == QNetworkReply::NoError)
        return;

    this->stop();
    emit error(DownloadFailed);
}

void MultithreadedDownloader::on_finished()
{
    m_finishedCount++;

    qDebug()<<"finishedCount:"<<m_finishedCount;

    if(m_finishedCount == m_threadCount)
    {
        if(m_writer->isRunning())           // Wait for the write finish
        {
            QEventLoop loop;
            QObject::connect(m_writer, &MultithreadedDownloaderWriter::finished, &loop,
                             &QEventLoop::quit);
            loop.exec();
        }
        m_finishedCount = 0;

        this->stop();
        this->updateProgress();

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
    for(DownloadMission *mission : m_missions)
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

    for(DownloadMission* mission : m_missions)
        bytesReceived += mission->downloadedSize();

    emit downloadProgress(bytesReceived, m_writer->size());
}

void MultithreadedDownloader::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    this->updateProgress();
}