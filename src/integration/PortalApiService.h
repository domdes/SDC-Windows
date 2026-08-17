#ifndef PORTALAPISERVICE_H
#define PORTALAPISERVICE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct UserProfileData {
    QString username;
    QString name;
    QString email;
    QString role = "guru";
    QString desa = "Cikunir";
};

struct MumbleProfileData {
    QString id;
    QString name;
    QString host;
    int port = 64738;
    QString password = "4622bekasiselatan";
    QString guruChannel = "Cikunir\\Relay 993";
    QString muridChannel = "Cikunir\\Relay 993";
    QString guruAccessToken;
    QString muridAccessToken;
    bool isPreinstalled = true;
};

class PortalApiService : public QObject {
    Q_OBJECT

public:
    explicit PortalApiService(QObject *parent = nullptr);

    void fetchUserProfile(const QString &email);
    void fetchAllMumbleConfigs();

signals:
    void profileFetched(const UserProfileData &profileData);
    void profilesFetched(const QList<MumbleProfileData> &profiles);
    void fetchFailed(const QString &errorMsg);

private:
    QNetworkAccessManager m_networkManager;
    UserProfileData m_currentData;
    QList<MumbleProfileData> m_profilesList;
};

#endif // PORTALAPISERVICE_H
