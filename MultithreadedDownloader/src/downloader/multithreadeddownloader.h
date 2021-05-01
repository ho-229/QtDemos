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

    /**
     * @brief 设置下载线程数
     * @param num 下载线程数
     */
    void setThreadNumber(int num){ m_threadNumber = num; }
    int threadNumber() const { return m_threadNumber; }

    /**
     * @brief 初始化下载
     * @note 获取下载文件大小，文件名
     * @return 是否成功
     */
    bool initDownload();

    /**
     * @brief 设置下载文件名
     * @param name 下载文件名
     */
    void setFileName(const QString& name){ m_writer->setFileName(name); }
    QString fileName() const { return m_writer->fileName(); }

    /**
     * @brief 设置下载文件目录
     * @param dir 下载文件目录
     */
    void setDownloadDir(const QString& dir){ m_writer->setDownloadDir(dir); }
    QString downloadDir() const { return m_writer->downloadDir(); }

    /**
     * @return 下载文件大小
     */
    qint64 downloadSize() const { return m_writer->size(); }

    QNetworkReply::NetworkError networkError() const { return m_networkError; }
    QString networkErrorString() const { return m_networkErrorString; }

    void start() Q_DECL_OVERRIDE;
    void pause() Q_DECL_OVERRIDE;
    void stop()  Q_DECL_OVERRIDE;

signals:
    void error(const MultithreadedDownloader::Error err);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void errorHanding(QNetworkReply::NetworkError err);     // 错误处理
    void on_finished();

private:
    QNetworkAccessManager* m_manager = nullptr;
    MultithreadedDownloaderWriter *m_writer = nullptr;

    int m_threadNumber = 0;
    int m_finishedCount = 0;

    int m_timerId = 0;

    QList<DownloadMission *> m_missions;

    QNetworkReply::NetworkError m_networkError = QNetworkReply::NoError;
    QString m_networkErrorString;

    inline DownloadMission* createMission(qint64 start, qint64 end);
    inline void destoryMissions();
    inline void updateProgress();
    inline void reset();

    void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE;        // 更新进度
};

#endif // MULTITHREADEDDOWNLOADER_H
