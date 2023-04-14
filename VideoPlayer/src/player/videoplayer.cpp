﻿/**
 * @brief Video Player
 * @anchor Ho 229
 * @date 2021/4/14
 */

#include "audiooutput.h"
#include "videoplayer.h"
#include "videoplayer_p.h"
#include "videorenderer.h"
#include "subtitlerenderer.h"

#include <QThread>
#include <QEventLoop>
#include <QMetaObject>
#include <QTimerEvent>

VideoPlayer::VideoPlayer(QQuickItem *parent) :
    QQuickFramebufferObject(parent),
    d_ptr(new VideoPlayerPrivate(this))
{
    Q_D(VideoPlayer);

    d->decoder = new FFmpegDecoder(nullptr);
    d->decoder->moveToThread(new QThread(this));
    d->decoder->thread()->start();

    d->audioOutput = new AudioOutput(d->decoder, this);

    d->subtitleRenderer = new SubtitleRenderer(this);

    QObject::connect(d->decoder, &FFmpegDecoder::activeVideoTrackChanged,
                     this, &VideoPlayer::activeVideoTrackChanged);
    QObject::connect(d->decoder, &FFmpegDecoder::activeAudioTrackChanged,
                     this, &VideoPlayer::activeAudioTrackChanged);
    QObject::connect(d->decoder, &FFmpegDecoder::activeSubtitleTrackChanged,
                     this, &VideoPlayer::activeSubtitleTrackChanged);
}

VideoPlayer::~VideoPlayer()
{
    Q_D(VideoPlayer);

    if(d->state != Stopped)
        this->stop();

    // Tell the decode thread exit
    d->decoder->thread()->quit();

    // Wait for finished
    if(!d->decoder->thread()->wait())
        FUNC_ERROR << ": Decode thread exit failed";

    // Delete the VideoPlayerPrivate
    delete d;
}

QQuickFramebufferObject::Renderer *VideoPlayer::createRenderer() const
{
    return new VideoRenderer(d_ptr);    // Create custom renderer
}

void VideoPlayer::setSource(const QUrl& source)
{
    Q_D(VideoPlayer);

    if(!source.isValid())
        return;

    d->decoder->setUrl(source);

    emit sourceChanged(source);
}

QUrl VideoPlayer::source() const
{
    return d_ptr->decoder->url();
}

VideoPlayer::State VideoPlayer::playState() const
{
    return d_ptr->state;
}

void VideoPlayer::play()
{
    Q_D(VideoPlayer);

    if(d->state == Playing)
        return;
    else if(d->state == Stopped)
    {
        if(d->decoder->state() == FFmpegDecoder::Closed)
        {
            QEventLoop loop;
            QObject::connect(d->decoder, &FFmpegDecoder::stateChanged, &loop,
                             [&](FFmpegDecoder::State state) {
                                 loop.exit(state != FFmpegDecoder::State::Opened);
                             });
            QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::load, Qt::QueuedConnection);

            if(!loop.exec())
            {
                d->isVideoInfoChanged = true;
                d->audioOutput->setAudioFormat(d->decoder->audioFormat());

                emit loaded();
            }
            else
            {
                emit errorOccurred(this->errorString());
                return;
            }
        }

        d->interval = static_cast<int>(1000 / d->decoder->fps());
        d->timerId = this->startTimer(d->interval);
        d->lastDiff = 0;
        d->totalStep = 0;

        d->audioOutput->play();
    }
    else if(d->state == Paused)
    {
        d->timerId = this->startTimer(d->interval);
        d->audioOutput->resume();
    }

    d->state = Playing;
    emit playStateChanged(Playing);
}

void VideoPlayer::pause()
{
    Q_D(VideoPlayer);

    if(d->state != Playing)
        return;

    this->killTimer(d->timerId);
    d->audioOutput->pause();

    d->state = Paused;
    emit playStateChanged(Paused);
}

void VideoPlayer::stop()
{
    Q_D(VideoPlayer);

    if(d->state == Stopped)
        return;
    else if(d->state == Playing)
        this->killTimer(d->timerId);

    d->audioOutput->stop();

    d->decoder->requestInterrupt();
    QEventLoop loop;
    QObject::connect(d->decoder, &FFmpegDecoder::stateChanged, &loop, &QEventLoop::quit);
    QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::release, Qt::QueuedConnection);
    loop.exec();

    emit positionChanged(0);

    this->update();
    d->subtitleRenderer->render({});

    d->state = Stopped;
    emit playStateChanged(Stopped);
}

void VideoPlayer::setVolume(qreal volume)
{
    Q_D(VideoPlayer);
    d->audioOutput->setVolume(volume);
    emit volumeChanged(volume);
}

qreal VideoPlayer::volume() const
{
    return d_ptr->audioOutput->volume();
}

void VideoPlayer::setActiveVideoTrack(int index)
{
    Q_D(VideoPlayer);

    if(!this->hasVideo() || d->decoder->activeVideoTrack() == index)
        return;

    d->decoder->requestInterrupt();
    QMetaObject::invokeMethod(d->decoder, "setActiveVideoTrack",
                              Qt::QueuedConnection, Q_ARG(int, index));
    QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::decode,
                              Qt::QueuedConnection);
}

int VideoPlayer::activeVideoTrack() const
{
    return d_ptr->decoder->activeVideoTrack();
}

void VideoPlayer::setActiveAudioTrack(int index)
{
    Q_D(VideoPlayer);

    if(!this->hasAudio() || d->decoder->activeAudioTrack() == index)
        return;

    d->decoder->requestInterrupt();
    QMetaObject::invokeMethod(d->decoder, "setActiveAudioTrack",
                              Qt::QueuedConnection, Q_ARG(int, index));
    QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::decode,
                              Qt::QueuedConnection);
}

int VideoPlayer::activeAudioTrack() const
{
    return d_ptr->decoder->activeAudioTrack();
}

void VideoPlayer::setActiveSubtitleTrack(int index)
{
    Q_D(VideoPlayer);

    if(!this->hasSubtitle() || d->decoder->activeSubtitleTrack() == index)
        return;

    d->decoder->requestInterrupt();
    QMetaObject::invokeMethod(d->decoder, "setActiveSubtitleTrack",
                              Qt::QueuedConnection, Q_ARG(int, index));
    QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::decode,
                              Qt::QueuedConnection);

    d->subtitleRenderer->render({});
}

int VideoPlayer::activeSubtitleTrack() const
{
    return d_ptr->decoder->activeSubtitleTrack();
}

int VideoPlayer::videoTrackCount() const
{
    return d_ptr->decoder->videoTrackCount();
}

int VideoPlayer::audioTrackCount() const
{
    return d_ptr->decoder->audioTrackCount();
}

int VideoPlayer::subtitleTrackCount() const
{
    return d_ptr->decoder->subtitleTrackCount();
}

int VideoPlayer::duration() const
{
    return d_ptr->decoder->duration();
}

int VideoPlayer::position() const
{
    return d_ptr->decoder->position();
}

bool VideoPlayer::hasVideo() const
{
    return d_ptr->decoder->videoTrackCount();
}

bool VideoPlayer::hasAudio() const
{
    return d_ptr->decoder->audioTrackCount();
}

bool VideoPlayer::hasSubtitle() const
{
    return d_ptr->decoder->subtitleTrackCount();
}

bool VideoPlayer::seekable() const
{
    return d_ptr->decoder->seekable();
}

QString VideoPlayer::errorString() const
{
    return d_ptr->decoder->errorString();
}

void VideoPlayer::seek(int position)
{
    Q_D(VideoPlayer);

    if(d->state == State::Stopped || !this->seekable())
        return;

    d->decoder->requestInterrupt();
    QMetaObject::invokeMethod(d->decoder, "seek", Qt::QueuedConnection, Q_ARG(int, position));

    d->lastDiff = 0;
    d->totalStep = 0;
}

void VideoPlayer::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(VideoPlayer);

    d->subtitleRenderer->setPosition(newGeometry.topLeft());
    d->subtitleRenderer->setSize(newGeometry.size());

    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
}

void VideoPlayer::timerEvent(QTimerEvent *)
{
    Q_D(VideoPlayer);

    d->isUpdated = true;
    this->update();

    if(d->decoder->isBitmapSubtitleActived())
        d->subtitleRenderer->render(d->decoder->takeSubtitleFrame());

    const int newPos = d->decoder->position();
    static int oldPos = 0;
    if(newPos != oldPos)
    {
        oldPos = newPos;
        emit positionChanged(newPos);
    }

    if(!d->decoder->hasFrame() && d->decoder->isEnd())
    {
        this->stop();
        return;
    }

    const qreal diff = d->decoder->diff();

    if(diff >= ALLOW_DIFF)          // Too slow
    {
        if(d->interval <= 10 || diff > ALLOW_DIFF * 2)
        {
            AVFrame *frame = d->decoder->takeVideoFrame();
            av_frame_free(&frame);
        }
        else
        {
            if(diff - d->lastDiff > ALLOW_DIFF / 2 && d->totalStep < 9)
            {
                ++d->totalStep;
                --d->interval;
                this->updateTimer();
            }
        }
    }
    else if(diff <= -ALLOW_DIFF)    // Too quick
    {
        if(diff - d->lastDiff > -ALLOW_DIFF / 2)
        {
            --d->totalStep;
            ++d->interval;
            this->updateTimer();
        }
    }

    d->lastDiff = diff;
}

void VideoPlayer::updateTimer()
{
    Q_D(VideoPlayer);

    this->killTimer(d->timerId);
    d->timerId = this->startTimer(d->interval);
}
