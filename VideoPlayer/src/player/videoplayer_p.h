/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2021/4/24
 */

#ifndef VIDEOPLAYERPRIVATE_H
#define VIDEOPLAYERPRIVATE_H

#include "videoplayer.h"

#include <QTimer>

class AudioOutput;
class VideoRenderer;
class FFmpegDecoder;

class VideoPlayerPrivate
{
public:
    VideoPlayerPrivate(VideoPlayer *parent) : q_ptr(parent) {}

    QTimer *updater = nullptr;

    QThread *decodeThread  = nullptr;
    FFmpegDecoder *decoder = nullptr;

    AudioOutput *audioOutput = nullptr;

    bool isPaused  = false;
    bool isPlaying = false;
    bool isUpdated = false;
    bool isVideoInfoChanged = false;

    int position = 0;
    qreal interval = 0;

    QQuickWindow* window() const { return q_ptr->window(); }

private:
    VideoPlayer * const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
