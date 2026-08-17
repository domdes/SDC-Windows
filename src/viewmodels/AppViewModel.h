#ifndef APPVIEWMODEL_H
#define APPVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>
#include "OAuthService.h"
#include "PortalApiService.h"
#include "EncryptedStorage.h"
#include "MumbleClientEngine.h"
#include "VoiceViewModel.h"

class AppViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentView READ currentView NOTIFY currentViewChanged)
    Q_PROPERTY(QString activeUserName READ activeUserName NOTIFY activeUserChanged)
    Q_PROPERTY(QString activeUserEmail READ activeUserEmail NOTIFY activeUserChanged)
    Q_PROPERTY(QString activeUserRole READ activeUserRole NOTIFY activeUserChanged)
    Q_PROPERTY(QString activeMumbleUsername READ activeMumbleUsername NOTIFY activeUserChanged)
    Q_PROPERTY(QVariantList preinstalledProfiles READ preinstalledProfiles NOTIFY profilesChanged)
    Q_PROPERTY(QVariantList savedProfiles READ savedProfiles NOTIFY profilesChanged)
    Q_PROPERTY(VoiceViewModel* voiceViewModel READ voiceViewModel CONSTANT)

public:
    explicit AppViewModel(QObject *parent = nullptr);

    QString currentView() const { return m_currentView; }
    QString activeUserName() const { return m_activeUserName; }
    QString activeUserEmail() const { return m_activeUserEmail; }
    QString activeUserRole() const { return m_activeUserRole; }
    QString activeMumbleUsername() const { return m_activeMumbleUsername; }
    QVariantList preinstalledProfiles() const { return m_preinstalledProfiles; }
    QVariantList savedProfiles() const { return m_savedProfiles; }
    VoiceViewModel* voiceViewModel() { return m_voiceViewModel; }

    Q_INVOKABLE void startGoogleLogin();
    Q_INVOKABLE void handleUriScheme(const QString &uri);
    Q_INVOKABLE void connectProfileById(const QString &profileId);
    Q_INVOKABLE void connectProfile(const QString &host, int port);
    Q_INVOKABLE void disconnectAndReturnToProfiles();
    Q_INVOKABLE void logout();

signals:
    void currentViewChanged();
    void activeUserChanged();
    void profilesChanged();

private:
    QString m_currentView = "SplashView";
    QString m_activeUserName;
    QString m_activeMumbleUsername;
    QString m_activeUserEmail;
    QString m_activeUserRole;

    QVariantList m_preinstalledProfiles;
    QVariantList m_savedProfiles;
    QList<MumbleProfileData> m_rawProfiles;

    OAuthService m_oauthService;
    PortalApiService m_portalService;
    MumbleClientEngine m_mumbleEngine;
    VoiceViewModel *m_voiceViewModel = nullptr;

    void autoLoginOrShowLogin();
    void populateProfiles(const QList<MumbleProfileData> &profiles);
};

#endif // APPVIEWMODEL_H
