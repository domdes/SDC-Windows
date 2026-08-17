#include "OAuthService.h"
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDebug>

static const char *SupabaseAnonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImZheXRob2NpaGx5em16cm53d2tzIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzYxNTYyOTcsImV4cCI6MjA5MTczMjI5N30.NurwRqdxe_oqnb8dNBVk8fT0P4vFxZBo1yimvw23Bws";

OAuthService::OAuthService(QObject *parent) : QObject(parent) {
    registerWindowsProtocol();

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &OAuthService::onNewTcpConnection);
}

OAuthService::~OAuthService() {
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void OAuthService::registerWindowsProtocol() {
#ifdef Q_OS_WIN
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    
    // Register sdcyajb protocol
    {
        QSettings registry("HKEY_CURRENT_USER\\Software\\Classes\\sdcyajb", QSettings::NativeFormat);
        registry.setValue(".", "URL:SDC YAJB Protocol");
        registry.setValue("URL Protocol", "");
        QSettings shell("HKEY_CURRENT_USER\\Software\\Classes\\sdcyajb\\shell\\open\\command", QSettings::NativeFormat);
        shell.setValue(".", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
    }

    // Register org.asyuhada.portal protocol
    {
        QSettings registry("HKEY_CURRENT_USER\\Software\\Classes\\org.asyuhada.portal", QSettings::NativeFormat);
        registry.setValue(".", "URL:Portal Asyuhada Protocol");
        registry.setValue("URL Protocol", "");
        QSettings shell("HKEY_CURRENT_USER\\Software\\Classes\\org.asyuhada.portal\\shell\\open\\command", QSettings::NativeFormat);
        shell.setValue(".", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
    }
#endif
}

QString OAuthService::generateCodeVerifier() {
    QByteArray buffer;
    buffer.resize(32);
    for (int i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return buffer.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString OAuthService::generateCodeChallenge(const QString &verifier) {
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

void OAuthService::startGoogleLogin() {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    // Start Local TCP Server on fixed port 54321
    if (!m_tcpServer->listen(QHostAddress::LocalHost, 54321)) {
        qWarning() << "[OAUTH SERVER] Port 54321 busy, listening on any port:" << m_tcpServer->errorString();
        m_tcpServer->listen(QHostAddress::LocalHost, 0);
    }
    m_localPort = m_tcpServer->serverPort();

    // Use official registered Supabase redirect URL matching Portal Android
    QString redirectUrl = "https://www.asyuhada-jaya.org/auth/portal-callback";
    QUrl authUrl("https://faythocihlyzmzrnwwks.supabase.co/auth/v1/authorize");
    QUrlQuery query;
    query.addQueryItem("provider", "google");
    query.addQueryItem("redirect_to", redirectUrl);
    query.addQueryItem("prompt", "select_account");
    authUrl.setQuery(query);

    qDebug() << "[OAUTH START] Opening browser for Google login:" << authUrl.toString();
    QDesktopServices::openUrl(authUrl);
}

void OAuthService::onNewTcpConnection() {
    QTcpSocket *socket = m_tcpServer->nextPendingConnection();
    if (!socket) return;

    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        QByteArray requestData = socket->readAll();
        QString requestStr = QString::fromUtf8(requestData);

        QString authCode;
        QString accessToken;
        QString refreshToken;

        // Check if query contains code
        if (requestStr.contains("code=")) {
            int codeIdx = requestStr.indexOf("code=");
            int endIdx = requestStr.indexOf(" ", codeIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("&", codeIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("\r\n", codeIdx);
            authCode = requestStr.mid(codeIdx + 5, endIdx - (codeIdx + 5));
        }

        // Check if query contains access_token directly (e.g. from /submit_token)
        if (requestStr.contains("access_token=")) {
            int tokIdx = requestStr.indexOf("access_token=");
            int endIdx = requestStr.indexOf(" ", tokIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("&", tokIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("\r\n", tokIdx);
            accessToken = requestStr.mid(tokIdx + 13, endIdx - (tokIdx + 13));
        }

        if (requestStr.contains("refresh_token=")) {
            int tokIdx = requestStr.indexOf("refresh_token=");
            int endIdx = requestStr.indexOf(" ", tokIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("&", tokIdx);
            if (endIdx == -1) endIdx = requestStr.indexOf("\r\n", tokIdx);
            refreshToken = requestStr.mid(tokIdx + 14, endIdx - (tokIdx + 14));
        }

        // Return CORS enabled HTTP 200 response
        QString htmlResponse =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: *\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Connection: close\r\n\r\n"
            "{\"status\":\"success\"}";

        socket->write(htmlResponse.toUtf8());
        socket->flush();
        socket->disconnectFromHost();

        if (!accessToken.isEmpty()) {
            qDebug() << "[OAUTH TOKEN RECEIVED FROM LOCAL SERVER] Fetching user profile...";
            fetchUserEmail(accessToken, refreshToken);
        } else if (!authCode.isEmpty()) {
            qDebug() << "[OAUTH CALLBACK CODE RECEIVED]:" << authCode;
            exchangeAuthorizationCode(authCode);
        }
    });
}

void OAuthService::exchangeAuthorizationCode(const QString &authCode) {
    QUrl tokenUrl("https://faythocihlyzmzrnwwks.supabase.co/auth/v1/token?grant_type=pkce");
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("apikey", SupabaseAnonKey);

    QJsonObject json;
    json["auth_code"] = authCode;
    json["code_verifier"] = m_codeVerifier;

    QNetworkReply *reply = m_networkManager.post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject res = QJsonDocument::fromJson(reply->readAll()).object();
            QString accessToken = res["access_token"].toString();
            QString refreshToken = res["refresh_token"].toString();
            QJsonObject userObj = res["user"].toObject();
            QString email = userObj["email"].toString();

            if (!email.isEmpty()) {
                qDebug() << "[OAUTH SUCCESS] User email from token response:" << email;
                emit loginSuccess(accessToken, refreshToken, email);
            } else if (!accessToken.isEmpty()) {
                fetchUserEmail(accessToken, refreshToken);
            } else {
                emit loginFailed("Respon token tidak valid");
            }
        } else {
            qWarning() << "[OAUTH TOKEN EXCHANGE ERROR]" << reply->errorString();
            emit loginFailed(QString("Gagal menukar kode otorisasi: %1").arg(reply->errorString()));
        }
    });
}

void OAuthService::fetchUserEmail(const QString &accessToken, const QString &refreshToken) {
    QUrl userUrl("https://faythocihlyzmzrnwwks.supabase.co/auth/v1/user");
    QNetworkRequest request(userUrl);
    request.setRawHeader("apikey", SupabaseAnonKey);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, accessToken, refreshToken]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject userObj = QJsonDocument::fromJson(reply->readAll()).object();
            QString email = userObj["email"].toString().trimmed();
            qDebug() << "[OAUTH USER FETCH SUCCESS] Authenticated email:" << email;
            if (!email.isEmpty()) {
                emit loginSuccess(accessToken, refreshToken, email);
            } else {
                emit loginFailed("Email akun Google tidak ditemukan.");
            }
        } else {
            qWarning() << "[OAUTH USER FETCH ERROR]" << reply->errorString();
            emit loginFailed("Gagal mengambil profil akun Google.");
        }
    });
}
