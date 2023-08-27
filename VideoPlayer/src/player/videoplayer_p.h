/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2021/4/24
 */

#ifndef VIDEOPLAYERPRIVATE_H
#define VIDEOPLAYERPRIVATE_H

#include "videoplayer.h"

#include <QElapsedTimer>

struct AVFrame;

class AudioOutput;
class FFmpegDecoder;
class VideoRenderer;

class Clock
{
    qreal m_time = 0;
    QElapsedTimer m_updateClock;
public:
    explicit Clock() = default;

    qreal time() const { return m_time + qreal(m_updateClock.elapsed()) / 1000; }
    bool isValid() const { return m_updateClock.isValid(); }

    void update(qreal time)
    {
        m_time = time;
        m_updateClock.start();
    }

    void pause()
    {
        if(m_updateClock.isValid())
            m_time += qreal(m_updateClock.elapsed()) / 1000;
    }
    void resume() { m_updateClock.start(); }

    void invalidate() { m_updateClock.invalidate(); }
};

class VideoPlayerPrivate
{
public:
    VideoPlayerPrivate(VideoPlayer *parent) : q_ptr(parent) {}

    FFmpegDecoder *decoder = nullptr;

    AudioOutput *audioOutput = nullptr;
    VideoRenderer *videoRenderer = nullptr;

    VideoPlayer::State state = VideoPlayer::Stopped;

    int position = 0;

    int interval = 0;
    int timerId = -1;

    AVFrame *audioFrame = nullptr;
    qint64 audioFramePos = 0;

    void restartAudioOutput();

    Clock videoClock;
    Clock audioClock;

    qint64 updateAudioData(char *data, qint64 maxlen);
    void updateVideoFrame();
    void updateSubtitleFrame();

private:
    inline void updateTimer(int newInterval);

    VideoPlayer *const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
