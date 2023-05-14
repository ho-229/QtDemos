/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2021/4/24
 */

#ifndef VIDEOPLAYERPRIVATE_H
#define VIDEOPLAYERPRIVATE_H

#include "videoplayer.h"

class AudioOutput;
class FFmpegDecoder;

class VideoPlayerPrivate
{
public:
    VideoPlayerPrivate(VideoPlayer *parent) : q_ptr(parent) {}

    FFmpegDecoder *decoder = nullptr;

    AudioOutput *audioOutput = nullptr;

    VideoPlayer::State state = VideoPlayer::Stopped;

    volatile bool isUpdated = false;
    volatile bool isFormatUpdated = false;

    int interval = 0;
    int maxInterval = 0;
    qreal averageInterval = 0;

    int timerId = -1;

    void updateTimer();
    void updateAudioOutput();

    /**
     * @brief Audio-to-video synchronization
     */
    void synchronize();

private:
    VideoPlayer *const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
