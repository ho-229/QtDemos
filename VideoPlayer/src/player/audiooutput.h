﻿/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QAudio>
#include <QObject>
#include <QAudioFormat>

class QAudioOutput;
class AudioDevice;
class FFmpegDecoder;

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(FFmpegDecoder *decoder, QObject *parent = nullptr);
    ~AudioOutput() Q_DECL_OVERRIDE;

    void setAudioFormat(const QAudioFormat format);

    void setVolume(qreal volume);
    qreal volume() const;

    void play();
    void pause();
    void resume();
    void stop();
    void reset();

signals:

private:
    QAudioOutput *m_output = nullptr;
    AudioDevice *m_audioDevice = nullptr;
};

#endif // AUDIOOUTPUT_H
