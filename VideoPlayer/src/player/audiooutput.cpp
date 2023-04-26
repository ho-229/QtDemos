/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#include "audiooutput.h"
#include "ffmpegdecoder.h"

#include <QIODevice>
#include <QAudioOutput>

class AudioDevice final : public QIODevice
{
    FFmpegDecoder *m_decoder = nullptr;
public:
    AudioDevice(FFmpegDecoder *decoder, QObject *parent = nullptr)
        : QIODevice(parent),
          m_decoder(decoder)
    { this->setOpenMode(QIODevice::ReadOnly); }

    qint64 readData(char *data, qint64 maxlen) override
    { return m_decoder->takeAudioData(data, maxlen); }

    qint64 writeData(const char *, qint64) override { return 0; }

    FFmpegDecoder *decoder() const { return m_decoder; }
};

AudioOutput::AudioOutput(FFmpegDecoder *decoder, QObject *parent) :
    QObject(parent)
{
    if(!decoder)
        FUNC_ERROR << ": Decoder is not valid";

    m_audioDevice = new AudioDevice(decoder, this);
}

AudioOutput::~AudioOutput()
{
    this->stop();
}

void AudioOutput::updateAudioOutput()
{
    if(m_output)
        this->stop();

    const auto format = m_audioDevice->decoder()->audioFormat();
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
        FUNC_ERROR << ": " << m_output->error();
}

void AudioOutput::pause()
{
    if (m_output)
        m_output->reset();
}

void AudioOutput::resume()
{
    if (m_output)
        m_output->start(m_audioDevice);
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
