#ifndef DOWNLOADMISSION_H
#define DOWNLOADMISSION_H

#include <QNetworkReply>

#include "abstractmission.h"
#include "multithreadeddownloaderwriter.h"

class QNetworkAccessManager;

class DownloadMission : public AbstractMission
{
    Q_OBJECT
public:
    explicit DownloadMission(QObject *parent);
    ~DownloadMission() Q_DECL_OVERRIDE;

    void setRange(qint64 start, qint64 end)
    {
        if(m_state == Stopped)
        {
            m_start = start;
            m_end = end;
            m_totalSize = end - start;
        }
    }

    void setManager(QNetworkAccessManager *manager){ m_manager = manager; }
    QNetworkAccessManager* manager(){ return m_manager; }

    void setWriter(MultithreadedDownloaderWriter *writer){ m_writer = writer; }
    MultithreadedDownloaderWriter* writer(){ return m_writer; }

    qint64 downloadedSize() const { return m_downloadedSize; }

    void start() Q_DECL_OVERRIDE;
    void pause() Q_DECL_OVERRIDE;
    void stop()  Q_DECL_OVERRIDE;

signals:
    void replyError(QNetworkReply::NetworkError err);

private slots:
    void on_finished();
    void writeData();

private:
    MultithreadedDownloaderWriter* m_writer = nullptr;
    QNetworkAccessManager* m_manager        = nullptr;
    QNetworkReply* m_reply                  = nullptr;

    qint64 m_start = -1;
    qint64 m_end   = -1;

    qint64 m_totalSize      = 0;
    qint64 m_downloadedSize = 0;

    inline void reset();
    inline void destoryReply();
};

#endif // DOWNLOADMISSION_H
