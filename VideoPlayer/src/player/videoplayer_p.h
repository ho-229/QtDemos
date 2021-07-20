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
class SubtitleRenderer;

class VideoPlayerPrivate
{
public:
    VideoPlayerPrivate(VideoPlayer *parent) : q_ptr(parent) {}

    QThread *decodeThread  = nullptr;
    FFmpegDecoder *decoder = nullptr;

    AudioOutput *audioOutput = nullptr;

    SubtitleRenderer *subtitleRenderer = nullptr;

    bool isPaused  = false;
    bool isPlaying = false;
    bool isUpdated = false;
    bool isVideoInfoChanged = false;

    int position = 0;

    int interval = 0;
    int timerId = -1;

    qreal lastDiff = 0, totalStep = 0;

    QQuickWindow* window() const { return q_ptr->window(); }

private:
    VideoPlayer * const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
