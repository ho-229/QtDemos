#include "multithreadeddownloaderwriter.h"

MultithreadedDownloaderWriter::MultithreadedDownloaderWriter(QObject *parent)
    : QThread(parent)
{
    QDir::setCurrent("./");
}

MultithreadedDownloaderWriter::~MultithreadedDownloaderWriter()
{
    if(m_downloadFile.isOpen())
        m_downloadFile.close();
}

void MultithreadedDownloaderWriter::write(QByteArray data, qint64 seek)
{
    WriteMisson newMisson = { data, seek };

    m_mutex.lock();
    m_writeList.push_back(newMisson);
    m_mutex.unlock();

    if(!this->isRunning())          // Stopped
        this->start();
}

void MultithreadedDownloaderWriter::run()
{
    do
    {
        m_downloadFile.seek(m_writeList.first().second);
        m_downloadFile.write(m_writeList.first().first);

        m_mutex.lock();
        m_writeList.pop_front();
        m_mutex.unlock();

        if(m_writeList.isEmpty())       // Hold on !!!
            this->msleep(100);
    }
    while(!m_writeList.isEmpty());
}
