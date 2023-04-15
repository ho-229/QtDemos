/**
 * @brief Video Player
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickFramebufferObject>

#define ALLOW_DIFF 0.03         // 40ms

class VideoPlayerPrivate;

class VideoPlayer : public QQuickFramebufferObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)

    Q_PROPERTY(int activeVideoTrack READ activeVideoTrack WRITE setActiveVideoTrack NOTIFY activeVideoTrackChanged)
    Q_PROPERTY(int activeAudioTrack READ activeAudioTrack WRITE setActiveAudioTrack NOTIFY activeAudioTrackChanged)
    Q_PROPERTY(int activeSubtitleTrack READ activeSubtitleTrack WRITE setActiveSubtitleTrack NOTIFY activeSubtitleTrackChanged)

    // Read only property
    Q_PROPERTY(int position READ position NOTIFY positionChanged)

    Q_PROPERTY(QString errorString READ errorString NOTIFY errorOccurred)
    Q_PROPERTY(State playState READ playState NOTIFY playStateChanged)

    Q_PROPERTY(int duration READ duration NOTIFY loaded)

    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY loaded)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY loaded)
    Q_PROPERTY(bool hasSubtitle READ hasSubtitle NOTIFY loaded)

    Q_PROPERTY(bool seekable READ seekable NOTIFY loaded)

    Q_PROPERTY(int videoTrackCount READ videoTrackCount NOTIFY loaded)
    Q_PROPERTY(int audioTrackCount READ audioTrackCount NOTIFY loaded)
    Q_PROPERTY(int subtitleTrackCount READ subtitleTrackCount NOTIFY loaded)

public:
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

    State playState() const;

    void setVolume(qreal volume);
    qreal volume() const;

    void setActiveVideoTrack(int index);
    int activeVideoTrack() const;

    void setActiveAudioTrack(int index);
    int activeAudioTrack() const;

    void setActiveSubtitleTrack(int index);
    int activeSubtitleTrack() const;

    int videoTrackCount() const;
    int audioTrackCount() const;
    int subtitleTrackCount() const;

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;
    int position() const;

    bool hasVideo() const;
    bool hasAudio() const;
    bool hasSubtitle() const;

    bool seekable() const;

    QString errorString() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

    Q_INVOKABLE void seek(int position);

signals:
    void errorOccurred(QString);

    void loaded();
    void sourceChanged(QUrl);
    void playStateChanged(VideoPlayer::State);
    void volumeChanged(qreal);
    void positionChanged(int);

    void activeVideoTrackChanged(int);
    void activeAudioTrackChanged(int);
    void activeSubtitleTrackChanged(int);

protected:
    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry) override;

private:
    VideoPlayerPrivate *const d_ptr;

    Q_DECLARE_PRIVATE(VideoPlayer)

    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

    inline void updateTimer();
};

#endif // VIDEOPLAYER_H
