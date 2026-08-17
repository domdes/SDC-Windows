#ifndef MUMBLECLIENTENGINE_H
#define MUMBLECLIENTENGINE_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QMap>
#include <QStringList>
#include <QVector>
#include "AudioEngine.h"

struct MumbleChannel {
    int id = 0;
    int parentId = -1;
    QString name;
    int position = 0;
    bool isExpanded = false;
};

struct MumbleUser {
    int session = 0;
    int channelId = 0;
    QString name;
    bool isMuted = false;
    bool isDeafened = false;
    bool isTalking = false;
    bool isSelf = false;
};

class MumbleClientEngine : public QObject {
    Q_OBJECT

public:
    explicit MumbleClientEngine(QObject *parent = nullptr);
    ~MumbleClientEngine();

    void connectToServer(const QString &host, int port = 64738, const QString &username = "Ken_Narottama", const QString &password = "4622bekasiselatan");
    void disconnectFromServer(bool intentional = true);

    bool isConnected() const { return m_isConnected; }
    int mySessionId() const { return m_mySessionId; }
    QMap<int, MumbleChannel> channels() const { return m_channels; }
    QMap<int, MumbleUser> users() const { return m_users; }

    void moveToChannel(int targetChannelId);
    void setChannelExpanded(int channelId, bool expanded);
    void setPttActive(bool active);
    void setAccessTokens(const QStringList &tokens) { m_accessTokens = tokens; }
    void setAutoJoinChannelName(const QString &chanName) { m_autoJoinChannelName = chanName; }
    void sendAccessTokens();
    void sendControlPacket(quint16 type, const QByteArray &payload);

signals:
    void stateUpdated();
    void connectionStateChanged(bool connected);
    void connectionError(const QString &errorMsg);
    void permissionDeniedOccurred(const QString &denyMsg);
    void logEmitted(const QString &logMsg);

private slots:
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onOpusFrameReady(const QByteArray &opusBytes);

private:
    QSslSocket *m_socket = nullptr;
    QTimer *m_pingTimer = nullptr;
    QTimer *m_talkingResetTimer = nullptr;
    QTimer *m_autoJoinRetryTimer = nullptr;

    AudioEngine m_audioEngine;

    bool m_isConnected = false;
    bool m_isIntentionalDisconnect = false;
    bool m_isPttActive = false;
    bool m_autoJoinPending = false;
    int m_autoJoinRetryCount = 0;
    int m_mySessionId = -1;
    int m_lastJoinedChannelId = -1;
    int m_reconnectAttempts = 0;
    int m_suffixIndex = 0;
    quint64 m_audioSequence = 0;

    QString m_lastHost;
    int m_lastPort = 64738;
    QString m_baseUsername;
    QString m_currentUsername;
    QString m_serverPassword;
    QString m_autoJoinChannelName;
    QStringList m_accessTokens;

    QByteArray m_readBuffer;
    QMap<int, MumbleChannel> m_channels;
    QMap<int, MumbleUser> m_users;

    void processControlPacket(quint16 type, const QByteArray &payload);
    void processAudioPacket(const QByteArray &payload);
    void parseChannelState(const QByteArray &payload);
    void parseChannelRemove(const QByteArray &payload);
    void parseUserState(const QByteArray &payload);
    void parseUserRemove(const QByteArray &payload);
    void parseServerSync(const QByteArray &payload);
    void parsePermissionDenied(const QByteArray &payload);
    void parseReject(const QByteArray &payload);
    void autoJoinResolvedChannel();
    void collapseAllExceptUserAncestors(int targetChannelId);
};

#endif // MUMBLECLIENTENGINE_H
