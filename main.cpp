#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "core/ffmpeg_player.h"
#include "core/player_controller.h"
#include "render/video_output_item.h"

int main(int argc, char *argv[])
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("TitanPlayer"));
    QCoreApplication::setApplicationName(QStringLiteral("TitanPlayer"));

    qRegisterMetaType<PlayState>("PlayState");
    qmlRegisterType<PlayerController>("TitanPlayer.Native", 1, 0, "PlayerController");
    qmlRegisterType<VideoOutputItem>("TitanPlayer.Native", 1, 0, "VideoOutputItem");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("TitanPlayer", "Main");

    return app.exec();
}
