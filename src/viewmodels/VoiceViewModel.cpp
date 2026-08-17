#include "VoiceViewModel.h"
#include "EncryptedStorage.h"
#include <QDateTime>
#include <QDebug>

VoiceViewModel::VoiceViewModel(MumbleClientEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine) {
    if (m_engine) {
        connect(m_engine, &MumbleClientEngine::stateUpdated, this, &VoiceViewModel::onEngineStateUpdated);
        connect(m_engine, &MumbleClientEngine::connectionStateChanged, this, &VoiceViewModel::onEngineConnectionStateChanged);
        connect(m_engine, &MumbleClientEngine::connectionError, this, &VoiceViewModel::onEngineConnectionError);
        connect(m_engine, &MumbleClientEngine::permissionDeniedOccurred, this, &VoiceViewModel::onEnginePermissionDenied);
        connect(m_engine, &MumbleClientEngine::logEmitted, this, &VoiceViewModel::onEngineLogEmitted);
    }

    // Load custom tokens from encrypted storage (trimmed without spaces)
    m_accessTokensStr = EncryptedStorage::readSecureString("custom_tokens");
    if (m_accessTokensStr.trimmed().isEmpty()) {
        m_accessTokensStr = "ustadz,relay,00_ 002_,guru,murid,cikunir,asrama,desa,354";
    }
    updateAccessTokens(m_accessTokensStr);
}

void VoiceViewModel::setUserRole(const QString &role) {
    if (m_userRole != role) {
        m_userRole = role;
        emit roleChanged();
    }
}

void VoiceViewModel::updateAccessTokens(const QString &tokensCsv) {
    QStringList rawList = tokensCsv.split(",", Qt::SkipEmptyParts);
    QStringList cleanList;
    for (const QString &t : rawList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !cleanList.contains(trimmed, Qt::CaseInsensitive)) {
            cleanList.append(trimmed);
        }
    }
    m_accessTokensStr = cleanList.join(",");
    EncryptedStorage::saveSecureString("custom_tokens", m_accessTokensStr);

    if (m_engine) {
        m_engine->setAccessTokens(cleanList);
    }
    emit accessTokensChanged();
}

void VoiceViewModel::connectToServer(const QString &host, int port, const QString &username, const QString &password) {
    if (!m_engine) return;
    m_isConnecting = true;
    m_statusText = "Menghubungkan ke server...";
    emit isConnectingChanged();
    emit statusTextChanged();

    QString pass = password.isEmpty() ? "4622bekasiselatan" : password;
    m_engine->connectToServer(host, port, username, pass);
}

void VoiceViewModel::disconnect() {
    if (!m_engine) return;
    m_isConnecting = false;
    m_statusText = "Terputus";
    emit isConnectingChanged();
    emit statusTextChanged();
    m_engine->disconnectFromServer(true);
    emit disconnected();
}

void VoiceViewModel::joinChannel(int channelId) {
    if (m_engine) {
        m_engine->moveToChannel(channelId);
    }
}

void VoiceViewModel::toggleChannelExpanded(int channelId) {
    if (m_engine) {
        auto channels = m_engine->channels();
        if (channels.contains(channelId)) {
            m_engine->setChannelExpanded(channelId, !channels[channelId].isExpanded);
        }
    }
}

void VoiceViewModel::setPttActive(bool active) {
    if (m_isPttActive != active) {
        m_isPttActive = active;
        if (m_engine) {
            m_engine->setPttActive(active);
        }
        emit pttStateChanged();
    }
}

void VoiceViewModel::togglePtt() {
    setPttActive(!m_isPttActive);
}

void VoiceViewModel::toggleLocalMute() {
    m_isLocalMuted = !m_isLocalMuted;
    if (m_isLocalMuted && m_isPttActive) {
        setPttActive(false);
    }
    emit stateUpdated();
}

void VoiceViewModel::toggleLocalDeafen() {
    m_isLocalDeafened = !m_isLocalDeafened;
    emit stateUpdated();
}

void VoiceViewModel::renameChannel(int channelId, const QString &newName) {
    Q_UNUSED(channelId);
    Q_UNUSED(newName);
}

void VoiceViewModel::clearDebugLogs() {
    m_debugLogs.clear();
    emit debugLogsChanged();
}

bool VoiceViewModel::isMuted() const {
    if (!m_engine) return false;
    int mySession = m_engine->mySessionId();
    auto users = m_engine->users();
    if (users.contains(mySession)) {
        return users[mySession].isMuted;
    }
    return false;
}

bool VoiceViewModel::isDeafened() const {
    if (!m_engine) return false;
    int mySession = m_engine->mySessionId();
    auto users = m_engine->users();
    if (users.contains(mySession)) {
        return users[mySession].isDeafened;
    }
    return false;
}

void VoiceViewModel::onEngineStateUpdated() {
    emit stateUpdated();
}

void VoiceViewModel::onEngineConnectionStateChanged(bool connected) {
    m_isConnecting = false;
    m_statusText = connected ? "Terhubung" : "Terputus";
    emit isConnectingChanged();
    emit statusTextChanged();
    emit stateUpdated();
}

void VoiceViewModel::onEngineConnectionError(const QString &errorMsg) {
    m_isConnecting = false;
    m_statusText = QString("Error: %1").arg(errorMsg);
    emit isConnectingChanged();
    emit statusTextChanged();
    emit connectionErrorOccurred(errorMsg);
}

void VoiceViewModel::onEnginePermissionDenied(const QString &denyMsg) {
    emit permissionDeniedAlert(denyMsg);
}

void VoiceViewModel::onEngineLogEmitted(const QString &logMsg) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_debugLogs.append(QString("[%1] %2\n").arg(timeStr, logMsg));
    emit debugLogsChanged();
}

QVariantMap VoiceViewModel::buildChannelNode(int channelId) const {
    QVariantMap node;
    if (!m_engine) return node;

    auto channels = m_engine->channels();
    auto users = m_engine->users();

    if (!channels.contains(channelId)) return node;
    const auto &c = channels[channelId];

    node["id"] = c.id;
    node["parentId"] = c.parentId;
    node["name"] = c.name;
    node["isExpanded"] = c.isExpanded;

    int mySession = m_engine->mySessionId();
    int myChannelId = -1;
    if (users.contains(mySession)) {
        myChannelId = users[mySession].channelId;
    }

    // Direct users in this channel
    QVariantList userList;
    int userCount = 0;
    bool isMyPath = (myChannelId == c.id);

    for (const auto &u : users) {
        if (u.channelId == c.id) {
            QVariantMap uMap;
            uMap["session"] = u.session;
            uMap["name"] = u.name;
            uMap["isMuted"] = u.isMuted;
            uMap["isDeafened"] = u.isDeafened;
            uMap["isTalking"] = u.isTalking;
            uMap["isSelf"] = u.isSelf;
            userList.append(uMap);
            userCount++;
        }
    }
    node["users"] = userList;

    // Subchannels (Recursive)
    QVariantList childrenList;
    for (const auto &sub : channels) {
        if (sub.parentId == c.id && sub.id != c.id) {
            QVariantMap childNode = buildChannelNode(sub.id);
            userCount += childNode["totalCount"].toInt();
            if (childNode["isMyPath"].toBool()) {
                isMyPath = true;
            }
            childrenList.append(childNode);
        }
    }
    node["children"] = childrenList;
    node["subChannels"] = childrenList;
    node["totalCount"] = userCount;
    node["isMyPath"] = isMyPath;

    return node;
}

QVariantList VoiceViewModel::channelTree() const {
    QVariantList tree;
    if (!m_engine) return tree;

    auto channels = m_engine->channels();
    // Root channels are channels with parentId < 0 or parentId == 0 (root itself)
    for (const auto &c : channels) {
        if (c.parentId < 0 || c.id == 0) {
            tree.append(buildChannelNode(c.id));
        }
    }
    return tree;
}
