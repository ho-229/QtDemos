﻿/**
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

    FFmpegDecoder *decoder = nullptr;

    AudioOutput *audioOutput = nullptr;

    SubtitleRenderer *subtitleRenderer = nullptr;

    VideoPlayer::State state = VideoPlayer::Stopped;

    volatile bool isUpdated = false;
    volatile bool isVideoInfoChanged = false;

    int interval = 0;
    qreal averageInterval = 0;
    int timerId = -1;

    qreal lastDiff = 0;

    QQuickWindow *window() const { return q_ptr->window(); }

    void updateTimer();
    void updateAudioOutput();

    void synchronize();

private:
    VideoPlayer *const q_ptr;
    Q_DECLARE_PUBLIC(VideoPlayer)
};

#endif // VIDEOPLAYERPRIVATE_H
