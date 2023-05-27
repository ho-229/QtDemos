/**
 * @brief Video Player
 * @anchor Ho 229
 * @date 2021/4/14
 */

#include "audiooutput.h"
#include "videoplayer.h"
#include "videoplayer_p.h"
#include "videorenderer.h"

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

    d->audioOutput = new AudioOutput([d](char *data, qint64 maxlen)
                                     { return d->updateAudioData(data, maxlen); }, this);

    QObject::connect(d->decoder, &FFmpegDecoder::activeVideoTrackChanged,
                     this, &VideoPlayer::activeVideoTrackChanged);
    QObject::connect(d->decoder, &FFmpegDecoder::activeAudioTrackChanged,
                     this, &VideoPlayer::activeAudioTrackChanged);
    QObject::connect(d->decoder, &FFmpegDecoder::activeSubtitleTrackChanged,
                     this, &VideoPlayer::activeSubtitleTrackChanged);
    QObject::connect(d->decoder, &FFmpegDecoder::positionChanged,
                     this, &VideoPlayer::positionChanged);

    QObject::connect(d->decoder, &FFmpegDecoder::activeAudioTrackChanged,
                     this, [this] { d_ptr->updateAudioOutput(); });
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
    d_ptr->videoRenderer = new VideoRenderer;
    return d_ptr->videoRenderer;    // Create custom renderer
}

void VideoPlayer::setSource(const QUrl& source)
{
    Q_D(VideoPlayer);

    d->decoder->setUrl(source);
    emit sourceChanged(source);
}

QUrl VideoPlayer::source() const
{
    return d_ptr->decoder->url();
}

VideoPlayer::State VideoPlayer::playbackState() const
{
    return d_ptr->state;
}

void VideoPlayer::play()
{
    Q_D(VideoPlayer);

    if(d->state == Playing)
        return;

    if(d->state == Stopped && d->decoder->state() == FFmpegDecoder::Closed)
    {
        QEventLoop loop;
        QObject::connect(d->decoder, &FFmpegDecoder::stateChanged, &loop, &QEventLoop::exit);
        QMetaObject::invokeMethod(d->decoder, &FFmpegDecoder::load, Qt::QueuedConnection);

        if(loop.exec() != FFmpegDecoder::Opened)
        {
            emit errorOccurred(this->errorString());
            return;
        }

        emit loaded();

        const auto fps = d->decoder->fps();
        d->averageInterval = qIsNaN(fps) ? 1000.0 : 1000 / fps;
        d->maxInterval = d->averageInterval * 2;
        d->interval = d->averageInterval;
    }

    d->timerId = this->startTimer(d->interval, Qt::PreciseTimer);
    d->audioOutput->play();

    d->state = Playing;
    emit playbackStateChanged(Playing);
}

void VideoPlayer::pause()
{
    Q_D(VideoPlayer);

    if(d->state != Playing)
        return;

    this->killTimer(d->timerId);
    d->audioOutput->pause();

    d->state = Paused;
    emit playbackStateChanged(Paused);
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

    d->videoRenderer->updateVideoFrame(nullptr);
    this->update();

    d->state = Stopped;
    emit playbackStateChanged(Stopped);
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

    if(d->interval != d->averageInterval)
    {
        d->interval = d->averageInterval;
        d->updateTimer();
    }
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

    if(d->interval != d->averageInterval)
    {
        d->interval = d->averageInterval;
        d->updateTimer();
    }
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

    if(d->interval != d->averageInterval)
    {
        d->interval = d->averageInterval;
        d->updateTimer();
    }
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

    if(d->state == State::Stopped || !this->seekable() || d->decoder->position() == position)
        return;

    d->decoder->requestInterrupt();
    QMetaObject::invokeMethod(d->decoder, "seek", Qt::QueuedConnection, Q_ARG(int, position));
    d->audioOutput->reset();

    if(d->interval != d->averageInterval)
    {
        d->interval = d->averageInterval;
        d->updateTimer();
    }
}

void VideoPlayer::timerEvent(QTimerEvent *)
{
    Q_D(VideoPlayer);

    // Update video frame
    AVFrame *frame = d->decoder->takeVideoFrame();
    if(frame)
        d->videoRenderer->updateVideoFrame(frame);
    d->videoRenderer->updateSubtitleFrame(d->decoder->takeSubtitleFrame());
    this->update();

    if(!d->decoder->hasFrame() && d->decoder->isEnd())
    {
        this->stop();
        return;
    }

    d->synchronize();
}
