﻿/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#include "audiooutput.h"
#include "ffmpegdecoder.h"

#include <QAudioOutput>

AudioOutput::AudioOutput(FFmpegDecoder *decoder, QObject *parent) :
    QObject(parent),
    m_decoder(decoder)
{
    if(!m_decoder)
        qCritical() << __FUNCTION__ << ": Decoder is not valid";
}

void AudioOutput::setAudioFormat(const QAudioFormat format)
{
    m_format = format;
}

void AudioOutput::setVolume(qreal volume)
{
    if(!m_output)
        return;

    m_output->setVolume(volume);
}

qreal AudioOutput::volume() const
{
    return m_output ? m_output->volume() : 0;
}

void AudioOutput::start()
{
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!m_decoder->hasAudio() || !info.isFormatSupported(m_format))
    {
        qCritical() << __FUNCTION__
                    << ": Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    m_output = new QAudioOutput(m_format, this);
    m_output->setBufferSize(65536);
    m_audioBuffer = m_output->start();

    QObject::connect(m_output, &QAudioOutput::stateChanged, this,
                     [this](QAudio::State state) {
                         if(state == QAudio::IdleState && m_decoder->hasFrame())
                         {
                             this->stop();
                             this->start();
                         }
                     });
}

void AudioOutput::pause(bool pause)
{
    if(!m_output)
        return;

    if(pause)
        m_output->suspend();
    else
        m_output->resume();
}

void AudioOutput::stop()
{
    if(!m_output)
        return;

    m_output->stop();
    m_output->deleteLater();
    m_output = nullptr;
}

void AudioOutput::update()
{
    if(m_output && m_output->bytesFree() > 4096)
        m_audioBuffer->write(m_decoder->takeAudioData(m_output->bytesFree()));
}