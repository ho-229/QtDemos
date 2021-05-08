/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2021/4/24
 */

#ifndef VIDEOPLAYERPRIVATE_H
#define VIDEOPLAYERPRIVATE_H

#include "videoplayer.h"

class AudioOutput;
class VideoRenderer;
class FFmpegDecoder;

class VideoPlayerPrivate
{
public:
    VideoPlayerPrivate(VideoPlayer *parent) : q_ptr(parent) {}

    FFmpegDecoder *decoder = nullptr;

    QThread *decodeThread = nullptr;
    AudioOutput *audioOutput = nullptr;

    bool isPaused  = false;
    bool isPlaying = false;
    bool isUpdated = false;
    bool isVideoInfoChanged = false;

    int timerId = 0;
    int position = 0;

    QQuickWindow* window() const { return q_ptr->window(); }

private:
    VideoPlayer * const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
