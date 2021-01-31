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

    bool openFile()
    { return m_downloadFile.open(QFile::WriteOnly) && m_downloadFile.resize(m_fileSize); }

    void closeFile()
    { m_downloadFile.close(); }

    void setFileName(const QString& name){ m_downloadFile.setFileName(name); }
    QString fileName(){ return m_downloadFile.fileName(); }

    void setDownloadDir(const QString& dir){ QDir::setCurrent(dir); }
    QString downloadDir(){ return QDir::currentPath(); }

    void setSize(qint64 size){ m_fileSize = size; }
    qint64 size(){ return m_fileSize; }

    void write(QByteArray data, qint64 seek);

signals:


private:
    void run() Q_DECL_OVERRIDE;

    qint64 m_fileSize = 0;
    QFile m_downloadFile;

    mutable QMutex m_mutex;
    QList<WriteMisson> m_writeList;

};

#endif // MULTITHREADEDDOWNLOADERWRITER_H
