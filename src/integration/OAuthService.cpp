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
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes\\sdcyajb", QSettings::NativeFormat);
    registry.setValue(".", "URL:SDC YAJB Protocol");
    registry.setValue("URL Protocol", "");
    
    QSettings shell("HKEY_CURRENT_USER\\Software\\Classes\\sdcyajb\\shell\\open\\command", QSettings::NativeFormat);
    shell.setValue(".", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
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

    // Start Local TCP Server on port 54321 (or fallback free port)
    if (!m_tcpServer->listen(QHostAddress::LocalHost, m_localPort)) {
        m_tcpServer->listen(QHostAddress::LocalHost, 0);
        m_localPort = m_tcpServer->serverPort();
    }

    m_codeVerifier = generateCodeVerifier();
    QString codeChallenge = generateCodeChallenge(m_codeVerifier);

    QString redirectUri = QString("http://localhost:%1/callback").arg(m_localPort);

    QUrl authUrl("https://faythocihlyzmzrnwwks.supabase.co/auth/v1/authorize");
    QUrlQuery query;
    query.addQueryItem("provider", "google");
    query.addQueryItem("redirect_to", redirectUri);
    query.addQueryItem("code_challenge", codeChallenge);
    query.addQueryItem("code_challenge_method", "S256");
    query.addQueryItem("response_type", "code");
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

        // Return HTML response that captures hash fragment (#access_token=...) if present
        QString htmlResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n\r\n"
            "<!DOCTYPE html><html><head><title>SDC YAJB - Login Google</title>"
            "<style>"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0F172A;color:#FFFFFF;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
            ".card{background:#1E293B;padding:40px 48px;border-radius:24px;box-shadow:0 20px 25px -5px rgba(0,0,0,0.5);text-align:center;max-width:440px;border:1px solid #334155;}"
            "h1{color:#10B981;font-size:22px;margin:0 0 12px 0;}"
            "p{color:#94A3B8;font-size:15px;line-height:1.5;margin:0 0 20px 0;}"
            ".badge{display:inline-block;padding:8px 16px;background:#065F46;color:#6EE7B7;border-radius:12px;font-size:13px;font-weight:600;}"
            "</style></head>"
            "<body><div class='card'>"
            "<h1>🌿 Login Google Berhasil</h1>"
            "<p>Autentikasi akun Google Anda telah terverifikasi.<br>Anda dapat menutup tab browser ini dan kembali ke aplikasi <b>SDC YAJB Desktop</b>.</p>"
            "<div class='badge'>✓ Terhubung ke SDC Desktop</div>"
            "</div>"
            "<script>"
            "if(window.location.hash && window.location.hash.length > 1){"
            "  var hash = window.location.hash.substring(1);"
            "  fetch('/submit_token?' + hash);"
            "}"
            "setTimeout(function(){ window.close(); }, 2500);"
            "</script></body></html>";

        socket->write(htmlResponse.toUtf8());
        socket->flush();
        socket->disconnectFromHost();

        if (!accessToken.isEmpty()) {
            qDebug() << "[OAUTH TOKEN RECEIVED] Directly fetching user profile...";
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
