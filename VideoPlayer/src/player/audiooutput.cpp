/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#include "audiooutput.h"

#include <QDebug>
#include <QIODevice>
#include <QAudioOutput>

class AudioDevice final : public QIODevice
{
    const AudioOutput::Callback m_callback;
public:
    AudioDevice(const AudioOutput::Callback &callback, QObject *parent = nullptr)
        : QIODevice(parent)
        , m_callback(callback)
    { this->setOpenMode(QIODevice::ReadOnly); }

    qint64 readData(char *data, qint64 maxlen) override
    {
        if(!maxlen)
            return 0;

        return m_callback(data, maxlen);
    }

    qint64 writeData(const char *, qint64) override { return 0; }
};

AudioOutput::AudioOutput(const Callback &callback, QObject *parent) :
    QObject(parent)
{
    m_audioDevice = new AudioDevice(callback, this);
}

AudioOutput::~AudioOutput()
{
    this->stop();
}

void AudioOutput::updateAudioOutput(const QAudioFormat &format)
{
    if(m_output)
        this->stop();

    if(format.isValid())
        m_output = new QAudioOutput(format, this);
}

void AudioOutput::setVolume(qreal volume)
{
    if (m_output)
        m_output->setVolume(volume);
}

qreal AudioOutput::volume() const
{
    return m_output ? m_output->volume() : 0.;
}

void AudioOutput::play()
{
    if (!m_output)
        return;

    m_output->start(m_audioDevice);

    if (m_output->error() != QAudio::NoError)
        qCritical() << __PRETTY_FUNCTION__ << ":" << m_output->error();
}

void AudioOutput::pause()
{
    if (m_output)
        m_output->reset();
}

void AudioOutput::stop()
{
    if (!m_output)
        return;

    m_output->reset();
    m_output->deleteLater();
    m_output = nullptr;
}

void AudioOutput::reset()
{
    if (!m_output)
        return;

    const auto previousState = m_output->state();
    m_output->reset();

    if (previousState == QAudio::ActiveState)
        m_output->start(m_audioDevice);
}

bool AudioOutput::isLowDataLeft() const
{
    if(!m_output)
        return false;

    return m_output->bytesFree() >= m_output->bufferSize() * 0.75;
}
