#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>
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

    const QString serverName = "SDCYAJB_SingleInstance_IPC";

    // 1. Check if another instance of SDC YAJB is already running
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        qDebug() << "[SINGLE INSTANCE] Another instance is already running. Forwarding argument and exiting...";
        QString arg = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QString("RAISE_WINDOW");
        socket.write(arg.toUtf8());
        socket.waitForBytesWritten(1000);
        socket.disconnectFromServer();
        return 0; // Terminate this secondary process immediately!
    }

    // 2. This is the primary instance -> Start QLocalServer for single-instance IPC
    QLocalServer localServer;
    QLocalServer::removeServer(serverName); // Clean up any stale pipe from crashed session
    if (!localServer.listen(serverName)) {
        qWarning() << "[SINGLE INSTANCE] Failed to start IPC server:" << localServer.errorString();
    } else {
        qDebug() << "[SINGLE INSTANCE] IPC server listening on:" << serverName;
    }

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

    // Listen for incoming arguments from secondary instances
    QObject::connect(&localServer, &QLocalServer::newConnection, [&localServer, &appViewModel, &engine]() {
        QLocalSocket *clientSocket = localServer.nextPendingConnection();
        if (!clientSocket) return;
        QObject::connect(clientSocket, &QLocalSocket::readyRead, [clientSocket, &appViewModel, &engine]() {
            QByteArray data = clientSocket->readAll();
            QString msg = QString::fromUtf8(data).trimmed();
            qDebug() << "[SINGLE INSTANCE IPC] Received data from second instance:" << msg;
            if (msg.startsWith("sdcyajb://") || msg.startsWith("org.asyuhada.portal://")) {
                appViewModel.handleUriScheme(msg);
            }
            // Bring main window to foreground
            const auto rootObjects = engine.rootObjects();
            if (!rootObjects.isEmpty()) {
                QWindow *window = qobject_cast<QWindow*>(rootObjects.first());
                if (window) {
                    window->show();
                    window->raise();
                    window->requestActivate();
                }
            }
        });
    });

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
