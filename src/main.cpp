#include "MusicPlayerController.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("GoodPlay 音乐播放器"));
    app.setOrganizationName(QStringLiteral("GoodPlayProject"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.png")));

    MusicPlayerController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("playerController"), &controller);
    engine.loadFromModule("MusicPlayer", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
