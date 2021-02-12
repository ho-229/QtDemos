/**
 * @brief DownloadMission
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "downloadmission.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

DownloadMission::DownloadMission(QObject *parent)
    : AbstractMission(parent)
{

}

DownloadMission::~DownloadMission()
{
    if(m_state == Running)
        this->DownloadMission::stop();
}

void DownloadMission::start()
{
    if(m_state == Running)
        return;

    QNetworkRequest request(m_url);

    if(m_start >= 0 && m_end > 0)
        request.setRawHeader("Range", QString("bytes=%1-%2")
                             .arg(m_start + m_downloadedSize).arg(m_end).toLatin1());
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    m_reply = m_manager->get(request);

    QObject::connect(m_reply, &QNetworkReply::finished, this,
                     &DownloadMission::on_finished);
#if QT_DEPRECATED_SINCE(5, 15)
    QObject::connect(m_reply, &QNetworkReply::errorOccurred, this,
                     &DownloadMission::replyError);
#else
    QObject::connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this,
                     &DownloadMission::replyError);
#endif
    QObject::connect(m_reply, &QNetworkReply::readyRead, this,
                     &DownloadMission::writeData);

    this->setFinished(false);
    this->updateState(Running);
}

void DownloadMission::pause()
{
    if(m_state == Running)
    {
        this->updateState(Paused);

        m_reply->abort();
        this->destoryReply();
    }
}

void DownloadMission::stop()
{
    if(m_state == Running)
    {
        this->updateState(Stopped);

        m_reply->abort();
        this->destoryReply();
        this->reset();
    }
}

void DownloadMission::on_finished()
{
    if(m_state == Running)
    {
        this->updateState(Stopped);
        this->setFinished();
    }
}

void DownloadMission::writeData()
{
    QByteArray data = m_reply->readAll();
    m_writer->write(data, m_start + m_downloadedSize);
    m_downloadedSize += data.size();
}

void DownloadMission::reset()
{
    m_start = -1;
    m_end   = -1;

    m_totalSize      = 0;
    m_downloadedSize = 0;
}

void DownloadMission::destoryReply()
{
    m_reply->deleteLater();
    m_reply = nullptr;
}
