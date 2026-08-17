#ifndef VOICEVIEWMODEL_H
#define VOICEVIEWMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include "MumbleClientEngine.h"

class VoiceViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList channelTree READ channelTree NOTIFY stateUpdated)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY stateUpdated)
    Q_PROPERTY(bool isConnecting READ isConnecting NOTIFY isConnectingChanged)
    Q_PROPERTY(bool isPttActive READ isPttActive NOTIFY pttStateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString debugLogs READ debugLogs NOTIFY debugLogsChanged)
    Q_PROPERTY(QString userRole READ userRole WRITE setUserRole NOTIFY roleChanged)
    Q_PROPERTY(QString accessTokens READ accessTokens WRITE updateAccessTokens NOTIFY accessTokensChanged)
    Q_PROPERTY(bool isMuted READ isMuted NOTIFY stateUpdated)
    Q_PROPERTY(bool isLocalMuted READ isLocalMuted NOTIFY stateUpdated)
    Q_PROPERTY(bool isDeafened READ isDeafened NOTIFY stateUpdated)
    Q_PROPERTY(bool isLocalDeafened READ isLocalDeafened NOTIFY stateUpdated)

public:
    explicit VoiceViewModel(MumbleClientEngine *engine, QObject *parent = nullptr);

    QVariantList channelTree() const;
    bool isConnected() const { return m_engine ? m_engine->isConnected() : false; }
    bool isConnecting() const { return m_isConnecting; }
    bool isPttActive() const { return m_isPttActive; }
    QString statusText() const { return m_statusText; }
    QString debugLogs() const { return m_debugLogs; }
    QString userRole() const { return m_userRole; }
    QString accessTokens() const { return m_accessTokensStr; }
    void setUserRole(const QString &role);

    bool isMuted() const;
    bool isLocalMuted() const { return m_isLocalMuted; }
    bool isDeafened() const;
    bool isLocalDeafened() const { return m_isLocalDeafened; }

    Q_INVOKABLE void connectToServer(const QString &host, int port, const QString &username, const QString &password = "4622bekasiselatan");
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE void joinChannel(int channelId);
    Q_INVOKABLE void toggleChannelExpanded(int channelId);
    Q_INVOKABLE void setPttActive(bool active);
    Q_INVOKABLE void togglePtt();
    Q_INVOKABLE void toggleLocalMute();
    Q_INVOKABLE void toggleLocalDeafen();
    Q_INVOKABLE void renameChannel(int channelId, const QString &newName);
    Q_INVOKABLE void updateAccessTokens(const QString &tokensCsv);
    Q_INVOKABLE void clearDebugLogs();

signals:
    void stateUpdated();
    void isConnectingChanged();
    void pttStateChanged();
    void statusTextChanged();
    void debugLogsChanged();
    void roleChanged();
    void accessTokensChanged();
    void disconnected();
    void connectionErrorOccurred(const QString &errorMsg);
    void permissionDeniedAlert(const QString &alertMsg);

private slots:
    void onEngineStateUpdated();
    void onEngineConnectionStateChanged(bool connected);
    void onEngineConnectionError(const QString &errorMsg);
    void onEnginePermissionDenied(const QString &denyMsg);
    void onEngineLogEmitted(const QString &logMsg);

private:
    MumbleClientEngine *m_engine = nullptr;
    bool m_isConnecting = false;
    bool m_isPttActive = false;
    bool m_isLocalMuted = false;
    bool m_isLocalDeafened = false;
    QString m_statusText = "Terputus";
    QString m_debugLogs;
    QString m_userRole = "guru";
    QString m_accessTokensStr;

    QVariantMap buildChannelNode(int channelId) const;
};

#endif // VOICEVIEWMODEL_H
