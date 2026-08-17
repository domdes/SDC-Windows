#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <iostream>
#include "AppViewModel.h"

void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(context);
    QString typeStr;
    switch (type) {
    case QtDebugMsg: typeStr = "DEBUG"; break;
    case QtWarningMsg: typeStr = "WARNING"; break;
    case QtCriticalMsg: typeStr = "CRITICAL"; break;
    case QtFatalMsg: typeStr = "FATAL"; break;
    }
    
    QString line = QString("[%1] [%2] %3\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"), typeStr, msg);
    std::cerr << line.toStdString();
    
    // Write to app directory log
    QString logPath = QDir(QCoreApplication::applicationDirPath()).filePath("app_debug.log");
    QFile outFile(logPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&outFile);
        ts << line;
        outFile.close();
    }
}

int main(int argc, char *argv[]) {
    // Set Qt Quick Controls style to Basic
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    qInstallMessageHandler(customLogHandler);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Yayasan Asyuhada Jaya Bekasi");
    app.setApplicationName("SDC YAJB");

    AppViewModel appViewModel;

    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.startsWith("sdcyajb://") || arg.startsWith("org.asyuhada.portal://")) {
            appViewModel.handleUriScheme(arg);
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appViewModel", &appViewModel);
    engine.rootContext()->setContextProperty("voiceViewModel", appViewModel.voiceViewModel());

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "CRITICAL: QQmlApplicationEngine failed to create root object for URL:" << objUrl;
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
