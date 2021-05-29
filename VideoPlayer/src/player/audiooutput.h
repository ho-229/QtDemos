/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QAudioFormat>

class QIODevice;
class QAudioOutput;
class FFmpegDecoder;

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(FFmpegDecoder * decoder, QObject *parent = nullptr);

    void setAudioFormat(const QAudioFormat format);
    QAudioFormat format() const { return m_format; }

    void setVolume(qreal volume);
    qreal volume() const;

    void start();
    void pause(bool pause);
    void stop();

    void update();

signals:

private:
    QAudioFormat m_format;
    QAudioOutput *m_output = nullptr;

    QIODevice *m_audioBuffer = nullptr;

    FFmpegDecoder *m_decoder = nullptr;
};

#endif // AUDIOOUTPUT_H
