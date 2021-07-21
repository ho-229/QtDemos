/**
 * @brief Video Player
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickFramebufferObject>

#define ALLOW_DIFF 0.04         // 40ms

class VideoPlayerPrivate;

class VideoPlayer : public QQuickFramebufferObject
{
    Q_OBJECT



    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)

    // Read only property
    Q_PROPERTY(int position READ position NOTIFY positionChanged)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)

    Q_PROPERTY(int duration READ duration NOTIFY loaded)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY loaded)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY loaded)
    Q_PROPERTY(int audioTrackCount READ audioTrackCount NOTIFY loaded)
    Q_PROPERTY(int subtitleTrackCount READ subtitleTrackCount NOTIFY loaded)

public:
    enum Error
    {
        ResourceError
    };

    Q_ENUM(Error)

    enum State
    {
        Playing,
        Paused,
        Stopped
    };

    Q_ENUM(State)

    VideoPlayer(QQuickItem *parent = nullptr);
    virtual ~VideoPlayer() Q_DECL_OVERRIDE;

    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    void setSource(const QUrl& source);
    QUrl source() const;

    State state() const;

    void setVolume(qreal volume);
    qreal volume() const;

    int audioTrackCount() const;
    int subtitleTrackCount() const;

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;

    int position() const;

    bool hasVideo() const;

    bool hasAudio() const;

    Q_INVOKABLE void play();

    Q_INVOKABLE void pause();

    Q_INVOKABLE void stop();

    Q_INVOKABLE void seek(int position);

    Q_INVOKABLE void trackedAudio(int index);

    Q_INVOKABLE void trackSubtitle(int index);

signals:
    void sourceChanged(QUrl source);

    void loaded();

    void error(const Error error);

    void stateChanged(const VideoPlayer::State state);
    void volumeChanged(const qreal volume);
    void positionChanged(const int position);

private:
    VideoPlayerPrivate * const d_ptr;

    Q_DECLARE_PRIVATE(VideoPlayer)

    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

    inline void updateTimer();
};

#endif // VIDEOPLAYER_H
