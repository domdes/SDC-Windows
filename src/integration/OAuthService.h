#ifndef OAUTHSERVICE_H
#define OAUTHSERVICE_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>

class OAuthService : public QObject {
    Q_OBJECT

public:
    explicit OAuthService(QObject *parent = nullptr);
    ~OAuthService();

    void registerWindowsProtocol();
    void startGoogleLogin();
    void exchangeAuthorizationCode(const QString &authCode);
    void fetchUserEmail(const QString &accessToken, const QString &refreshToken);

signals:
    void loginSuccess(const QString &accessToken, const QString &refreshToken, const QString &userEmail);
    void loginFailed(const QString &errorMsg);

private slots:
    void onNewTcpConnection();

private:
    QNetworkAccessManager m_networkManager;
    QTcpServer *m_tcpServer = nullptr;
    QString m_codeVerifier;
    quint16 m_localPort = 54321;

    QString generateCodeVerifier();
    QString generateCodeChallenge(const QString &verifier);
};

#endif // OAUTHSERVICE_H
