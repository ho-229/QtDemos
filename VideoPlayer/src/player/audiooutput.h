/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <functional>

#include <QObject>
#include <QAudioFormat>

class QAudioOutput;
class AudioDevice;

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<qint64(char *, qint64)>;

    explicit AudioOutput(const Callback &callback, QObject *parent = nullptr);
    ~AudioOutput() Q_DECL_OVERRIDE;

    /**
     * @brief Update audio output when the audio format changed
     */
    void updateAudioOutput(const QAudioFormat &format);

    void setVolume(qreal volume);
    qreal volume() const;

    void play();
    void pause();
    void stop();

    /**
     * @brief Reset the buffer of audio output
     */
    void reset();

    bool isLowDataLeft() const;
    qreal bufferDuration() const;

private:
    QAudioOutput *m_output = nullptr;
    AudioDevice *m_audioDevice = nullptr;
    qreal m_bufferDuration = 0;
};

#endif // AUDIOOUTPUT_H
