/**
 * @brief MultithreadedDownloaderWriter
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef MULTITHREADEDDOWNLOADERWRITER_H
#define MULTITHREADEDDOWNLOADERWRITER_H

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QThread>

typedef QPair<QByteArray, qint64> WriteMisson;

class MultithreadedDownloaderWriter : public QThread
{
    Q_OBJECT
public:
    explicit MultithreadedDownloaderWriter(QObject *parent = nullptr);
    ~MultithreadedDownloaderWriter() Q_DECL_OVERRIDE;

    /**
     * @brief 打开文件
     * @return 操作是否成功
     */
    bool openFile()
    { return m_downloadFile.open(QFile::WriteOnly) && m_downloadFile.resize(m_fileSize); }

    /**
     * @brief 关闭文件
     */
    void closeFile()
    { m_downloadFile.close(); }

    /**
     * @brief 设置下载文件名
     * @param name 下载文件名
     */
    void setFileName(const QString& name){ m_downloadFile.setFileName(name); }
    QString fileName(){ return m_downloadFile.fileName(); }

    /**
     * @brief 设置下载文件目录
     * @param dir 下载文件目录
     */
    void setDownloadDir(const QString& dir){ QDir::setCurrent(dir); }
    QString downloadDir(){ return QDir::currentPath(); }

    /**
     * @brief 设置下载文件大小
     * @param size 下载文件大小 byte
     */
    void setSize(qint64 size){ m_fileSize = size; }
    qint64 size(){ return m_fileSize; }

    /**
     * @brief 添加写任务
     * @param data 写入数据
     * @param seek 写入偏移量
     */
    void write(const QByteArray& data, const qint64 seek);

private:
    void run() Q_DECL_OVERRIDE;

    qint64 m_fileSize = 0;
    QFile m_downloadFile;

    mutable QMutex m_mutex;
    QList<WriteMisson> m_writeList;
};

#endif // MULTITHREADEDDOWNLOADERWRITER_H
