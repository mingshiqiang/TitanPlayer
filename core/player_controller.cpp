#include "player_controller.h"

#include "../render/video_frame_source.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_player(new FFmpegPlayer(this))
    , m_audioOutput(new AudioOutput(this))
    , m_frameSource(new VideoFrameSource(this))
{
    connect(m_player, &FFmpegPlayer::positionChanged, this, &PlayerController::setPosition);
    connect(m_player, &FFmpegPlayer::durationChanged, this, &PlayerController::setDuration);
    connect(m_player, &FFmpegPlayer::stateChanged, this, [this](PlayState state) {
        setPlaying(state == PlayState::Playing);
    });
    connect(m_player, &FFmpegPlayer::speedChanged, this, [this](double value) {
        if (!qFuzzyCompare(m_speed, value)) {
            m_speed = value;
            emit speedChanged();
        }
    });
    connect(m_player, &FFmpegPlayer::finished, this, [this]() {
        if (m_items.size() > 1) {
            next();
        } else {
            setPlaying(false);
            setPosition(0);
        }
    });
    connect(m_player, &FFmpegPlayer::errorOccurred, this, &PlayerController::setStatusMessage);

    m_frameTimer.setInterval(15);
    connect(&m_frameTimer, &QTimer::timeout, this, &PlayerController::drainVideoFrames);
    m_frameTimer.start();
}

PlayerController::~PlayerController()
{
    m_player->stop();
    if (m_player->isRunning()) {
        m_player->wait(3000);
    }
}

QObject *PlayerController::videoFrameSource() const
{
    return m_frameSource;
}

QVariantList PlayerController::playlist() const
{
    QVariantList list;
    for (int i = 0; i < m_items.size(); ++i) {
        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("filePath"), m_items[i].filePath);
        item.insert(QStringLiteral("fileName"), m_items[i].fileName);
        item.insert(QStringLiteral("duration"), m_items[i].durationMs);
        item.insert(QStringLiteral("current"), i == m_currentIndex);
        list.append(item);
    }
    return list;
}

QString PlayerController::currentFileName() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_items.size()) {
        return m_items[m_currentIndex].fileName;
    }
    return QString();
}

QString PlayerController::screenshotDir() const
{
    QSettings settings;
    const QString value = settings.value(QStringLiteral("screenshot/dir")).toString();
    return value.isEmpty() ? defaultScreenshotDir() : value;
}

QString PlayerController::screenshotFormat() const
{
    QSettings settings;
    const QString fmt = settings.value(QStringLiteral("screenshot/format"), QStringLiteral("png")).toString().toLower();
    return fmt == QStringLiteral("jpg") ? QStringLiteral("jpg") : QStringLiteral("png");
}

void PlayerController::openUrls(const QList<QUrl> &urls)
{
    QList<PlaylistItem> items;
    for (const QUrl &url : urls) {
        const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
        if (path.isEmpty()) {
            continue;
        }
        PlaylistItem item;
        item.filePath = path;
        item.fileName = QFileInfo(path).fileName();
        items.append(item);
    }

    if (items.isEmpty()) {
        return;
    }

    m_items = items;
    emit playlistChanged();
    setCurrentIndex(0);
    openFile(m_items.first().filePath);
}

void PlayerController::play()
{
    if (m_player->state() == PlayState::Paused) {
        m_player->play();
        if (m_audioStarted) {
            m_audioOutput->resume();
        }
        return;
    }

    if (m_player->state() == PlayState::Stopped && m_currentIndex >= 0 && m_currentIndex < m_items.size()) {
        openFile(m_items[m_currentIndex].filePath);
    }
}

void PlayerController::pause()
{
    if (m_player->state() == PlayState::Playing) {
        m_player->pause();
        if (m_audioStarted) {
            m_audioOutput->suspend();
        }
    }
}

void PlayerController::togglePlay()
{
    if (m_player->state() == PlayState::Playing) {
        pause();
    } else {
        play();
    }
}

void PlayerController::seek(qint64 positionMs)
{
    if (m_duration <= 0) {
        return;
    }
    positionMs = qBound<qint64>(0, positionMs, m_duration);
    m_player->seek(positionMs);
    setPosition(positionMs);
}

void PlayerController::previous()
{
    if (m_items.isEmpty()) {
        return;
    }
    const int previousIndex = (m_currentIndex <= 0) ? m_items.size() - 1 : m_currentIndex - 1;
    setCurrentIndex(previousIndex);
    openFile(m_items[previousIndex].filePath);
}

void PlayerController::next()
{
    if (m_items.isEmpty()) {
        return;
    }
    const int nextIndex = (m_currentIndex < 0 || m_currentIndex >= m_items.size() - 1) ? 0 : m_currentIndex + 1;
    setCurrentIndex(nextIndex);
    openFile(m_items[nextIndex].filePath);
}

void PlayerController::playAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    setCurrentIndex(index);
    openFile(m_items[index].filePath);
}

void PlayerController::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    if (m_volume == volume) {
        return;
    }
    m_volume = volume;
    m_player->setVolume(volume);
    m_audioOutput->setVolume(volume);
    emit volumeChanged();
}

void PlayerController::setSpeed(double speed)
{
    speed = qBound(0.25, speed, 4.0);
    if (qFuzzyCompare(m_speed, speed)) {
        return;
    }
    m_speed = speed;
    m_player->setSpeed(speed);
    emit speedChanged();
}

void PlayerController::takeScreenshot()
{
    QImage frame = m_frameSource->frame();
    if (frame.isNull()) {
        setStatusMessage(tr("No video frame to capture."));
        return;
    }

    const QString dir = screenshotDir();
    const QString fmt = screenshotFormat();
    QDir().mkpath(dir);

    QString base = currentFileName();
    if (base.isEmpty()) {
        base = QStringLiteral("screenshot");
    }
    base = QFileInfo(base).completeBaseName();

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    const QString path = QDir(dir).filePath(QStringLiteral("%1_%2.%3").arg(base, timestamp, fmt));

    QImageWriter writer(path, fmt.toLatin1());
    bool ok = false;
    if (fmt == QStringLiteral("jpg")) {
        writer.setQuality(92);
        QImage flat(frame.size(), QImage::Format_RGB32);
        flat.fill(Qt::black);
        QPainter painter(&flat);
        painter.drawImage(0, 0, frame.convertToFormat(QImage::Format_RGB32));
        painter.end();
        ok = writer.write(flat);
    } else {
        ok = writer.write(frame);
    }

    setStatusMessage(ok ? tr("Saved: %1").arg(QDir::toNativeSeparators(path))
                        : tr("Failed to save screenshot."));
}

void PlayerController::setScreenshotSettings(const QUrl &folderUrl, const QString &format)
{
    QString folder = folderUrl.isLocalFile() ? folderUrl.toLocalFile() : folderUrl.toString();
    if (folder.isEmpty()) {
        folder = screenshotDir();
    }

    const QString normalizedFormat = format.toLower() == QStringLiteral("jpg") ? QStringLiteral("jpg") : QStringLiteral("png");
    QSettings settings;
    settings.setValue(QStringLiteral("screenshot/dir"), folder);
    settings.setValue(QStringLiteral("screenshot/format"), normalizedFormat);
    emit screenshotSettingsChanged();
}

void PlayerController::clearStatusMessage()
{
    setStatusMessage(QString());
}

bool PlayerController::openFile(const QString &path)
{
    if (m_player->state() != PlayState::Stopped) {
        m_player->stop();
        if (m_player->isRunning()) {
            m_player->wait(3000);
        }
    }

    m_audioOutput->stop();
    m_audioStarted = false;
    m_frameSource->clear();
    setPosition(0);
    setDuration(0);

    if (!m_player->open(path)) {
        setPlaying(false);
        return false;
    }

    if (m_player->hasAudio()) {
        m_audioOutput->start(m_player->audioSampleRate(), m_player->audioChannels(), m_player->audioQueue());
        const QAudioFormat &format = m_audioOutput->format();
        m_player->setAudioOutputFormat(format.sampleRate(), format.channelCount(), format.sampleFormat());
        m_audioOutput->setVolume(m_volume);
        m_audioStarted = true;
    }

    m_player->setSpeed(m_speed);
    m_player->setVolume(m_volume);
    m_player->play();
    setStatusMessage(tr("Playing: %1").arg(QFileInfo(path).fileName()));
    return true;
}

void PlayerController::setCurrentIndex(int index)
{
    if (m_currentIndex == index) {
        return;
    }
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentFileChanged();
    emit playlistChanged();
}

void PlayerController::setPlaying(bool playing)
{
    if (m_playing == playing) {
        return;
    }
    m_playing = playing;
    emit playingChanged();
}

void PlayerController::setPosition(qint64 position)
{
    if (m_position == position) {
        return;
    }
    m_position = position;
    emit positionChanged();
}

void PlayerController::setDuration(qint64 duration)
{
    if (m_duration == duration) {
        return;
    }
    m_duration = duration;
    if (m_currentIndex >= 0 && m_currentIndex < m_items.size()) {
        m_items[m_currentIndex].durationMs = duration;
        emit playlistChanged();
    }
    emit durationChanged();
}

void PlayerController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

void PlayerController::drainVideoFrames()
{
    if (!m_player) {
        return;
    }

    VideoFrame frame;
    bool hasFrame = false;
    while (m_player->videoQueue()->pop(frame, 0)) {
        hasFrame = true;
    }

    if (hasFrame) {
        m_frameSource->setFrame(frame.image);
    }
}

QString PlayerController::defaultScreenshotDir()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return pictures + QDir::separator() + QStringLiteral("TitanPlayer") + QDir::separator() + QStringLiteral("Screenshots");
}
