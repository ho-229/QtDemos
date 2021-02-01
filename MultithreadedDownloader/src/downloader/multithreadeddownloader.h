/**
 * @brief MultithreadedDownloader
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef MULTITHREADEDDOWNLOADER_H
#define MULTITHREADEDDOWNLOADER_H

#include <QFile>

#include "downloadmission.h"
#include "multithreadeddownloaderwriter.h"

class QBasicTimer;
class QNetworkAccessManager;

class MultithreadedDownloader : public AbstractMission
{
    Q_OBJECT
public:
    enum Error
    {
        OpenFileFailed,
        DownloadFailed
    };

    explicit MultithreadedDownloader(QObject *parent = nullptr);
    ~MultithreadedDownloader() Q_DECL_OVERRIDE;

    void setThreadCount(int threadCount);
    int threadCount(){ return m_threadCount; }

    bool getFileInfo();             // 获取文件信息

    void setFileName(const QString& name){ m_writer->setFileName(name); }
    QString fileName(){ return m_writer->fileName(); }

    void setDownloadDir(const QString& dir){ m_writer->setDownloadDir(dir); }
    QString downloadDir(){ return m_writer->downloadDir(); }

    qint64 fileSize(){ return m_writer->size(); }

    QNetworkReply::NetworkError networkError() const { return m_networkError; }
    QString networkErrorString() const { return m_networkErrorString; }

    void start() Q_DECL_OVERRIDE;
    void pause() Q_DECL_OVERRIDE;
    void stop()  Q_DECL_OVERRIDE;

signals:
    void error(Error err);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void errorHanding(QNetworkReply::NetworkError err);
    void on_finished();

private:
    QNetworkAccessManager* m_manager = nullptr;
    MultithreadedDownloaderWriter *m_writer = nullptr;

    int m_threadCount = 0;
    int m_finishedCount = 0;

    int m_timerId = 0;

    QList<DownloadMission *>m_missions;

    QNetworkReply::NetworkError m_networkError = QNetworkReply::NoError;
    QString m_networkErrorString;

    inline DownloadMission* createMission(qint64 start, qint64 end);
    inline void destoryMissions();
    inline void updateProgress();

    void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE;
};

#endif // MULTITHREADEDDOWNLOADER_H
