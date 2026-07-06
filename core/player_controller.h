#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include "audio_output.h"
#include "ffmpeg_player.h"

class VideoFrameSource;

class PlayerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *videoFrameSource READ videoFrameSource CONSTANT)
    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString screenshotDir READ screenshotDir NOTIFY screenshotSettingsChanged)
    Q_PROPERTY(QString screenshotFormat READ screenshotFormat NOTIFY screenshotSettingsChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);
    ~PlayerController() override;

    QObject *videoFrameSource() const;
    QVariantList playlist() const;
    int currentIndex() const { return m_currentIndex; }
    QString currentFileName() const;
    bool isPlaying() const { return m_playing; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    int volume() const { return m_volume; }
    double speed() const { return m_speed; }
    QString statusMessage() const { return m_statusMessage; }
    QString screenshotDir() const;
    QString screenshotFormat() const;

    Q_INVOKABLE void openUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void takeScreenshot();
    Q_INVOKABLE void setScreenshotSettings(const QUrl &folderUrl, const QString &format);
    Q_INVOKABLE void clearStatusMessage();

public slots:
    void setVolume(int volume);
    void setSpeed(double speed);

signals:
    void playlistChanged();
    void currentIndexChanged();
    void currentFileChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void speedChanged();
    void statusMessageChanged();
    void screenshotSettingsChanged();

private:
    struct PlaylistItem {
        QString filePath;
        QString fileName;
        qint64 durationMs = 0;
    };

    bool openFile(const QString &path);
    void setCurrentIndex(int index);
    void setPlaying(bool playing);
    void setPosition(qint64 position);
    void setDuration(qint64 duration);
    void setStatusMessage(const QString &message);
    void drainVideoFrames();
    static QString defaultScreenshotDir();

    FFmpegPlayer *m_player = nullptr;
    AudioOutput *m_audioOutput = nullptr;
    VideoFrameSource *m_frameSource = nullptr;
    QTimer m_frameTimer;

    QList<PlaylistItem> m_items;
    int m_currentIndex = -1;
    bool m_playing = false;
    bool m_audioStarted = false;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volume = 100;
    double m_speed = 1.0;
    QString m_statusMessage;
};

#endif // PLAYER_CONTROLLER_H
