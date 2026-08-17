#include "MumbleClientEngine.h"
#include "MumbleProtobuf.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

static void writeMumbleVarint(QByteArray &buffer, quint64 value) {
    if (value < 0x80) {
        buffer.append(static_cast<char>(value));
    } else if (value < 0x4000) {
        buffer.append(static_cast<char>((value >> 8) | 0x80));
        buffer.append(static_cast<char>(value & 0xFF));
    } else if (value < 0x200000) {
        buffer.append(static_cast<char>((value >> 16) | 0xC0));
        buffer.append(static_cast<char>((value >> 8) & 0xFF));
        buffer.append(static_cast<char>(value & 0xFF));
    } else {
        buffer.append(static_cast<char>((value >> 24) | 0xE0));
        buffer.append(static_cast<char>((value >> 16) & 0xFF));
        buffer.append(static_cast<char>((value >> 8) & 0xFF));
        buffer.append(static_cast<char>(value & 0xFF));
    }
}

static quint64 readMumbleVarint(const QByteArray &buffer, int &offset) {
    if (offset >= buffer.size()) return 0;
    quint8 b = static_cast<quint8>(buffer[offset++]);
    if ((b & 0x80) == 0x00) {
        return b;
    } else if ((b & 0xC0) == 0x80) {
        if (offset >= buffer.size()) return 0;
        return ((b & 0x3F) << 8) | static_cast<quint8>(buffer[offset++]);
    } else if ((b & 0xE0) == 0xC0) {
        if (offset + 1 >= buffer.size()) return 0;
        quint64 val = ((b & 0x1F) << 16) | (static_cast<quint8>(buffer[offset]) << 8) | static_cast<quint8>(buffer[offset + 1]);
        offset += 2;
        return val;
    } else if ((b & 0xF0) == 0xE0) {
        if (offset + 2 >= buffer.size()) return 0;
        quint64 val = ((b & 0x0F) << 24) | (static_cast<quint8>(buffer[offset]) << 16) | (static_cast<quint8>(buffer[offset + 1]) << 8) | static_cast<quint8>(buffer[offset + 2]);
        offset += 3;
        return val;
    } else if ((b & 0xF0) == 0xF0) {
        switch (b & 0xFC) {
            case 0xF0: {
                if (offset + 3 >= buffer.size()) return 0;
                quint64 val = (static_cast<quint8>(buffer[offset]) << 24) | (static_cast<quint8>(buffer[offset + 1]) << 16) | (static_cast<quint8>(buffer[offset + 2]) << 8) | static_cast<quint8>(buffer[offset + 3]);
                offset += 4;
                return val;
            }
            case 0xF4: {
                if (offset + 7 >= buffer.size()) return 0;
                quint64 val = 0;
                for (int i = 0; i < 8; ++i) {
                    val = (val << 8) | static_cast<quint8>(buffer[offset++]);
                }
                return val;
            }
            default:
                return ~(b & 0x03);
        }
    }
    return 0;
}

MumbleClientEngine::MumbleClientEngine(QObject *parent)
    : QObject(parent) {
    m_socket = new QSslSocket(this);
    m_pingTimer = new QTimer(this);
    m_talkingResetTimer = new QTimer(this);
    m_autoJoinRetryTimer = new QTimer(this);

    connect(m_socket, &QSslSocket::encrypted, this, &MumbleClientEngine::onSocketConnected);
    connect(m_socket, &QSslSocket::readyRead, this, &MumbleClientEngine::onSocketReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &MumbleClientEngine::onSocketError);

    connect(&m_audioEngine, &AudioEngine::opusFrameReady, this, &MumbleClientEngine::onOpusFrameReady);

    connect(m_pingTimer, &QTimer::timeout, [this]() {
        if (m_isConnected && m_socket->state() == QAbstractSocket::ConnectedState) {
            MumbleBufferWriter pingWriter;
            quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
            pingWriter.writeVarintField(1, now);
            sendControlPacket(3, pingWriter.toByteArray()); // Type 3 = Ping
        }
    });

    connect(m_talkingResetTimer, &QTimer::timeout, [this]() {
        bool changed = false;
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.value().isTalking && !it.value().isSelf) {
                it.value().isTalking = false;
                changed = true;
            }
        }
        if (changed) {
            emit stateUpdated();
        }
    });

    connect(m_autoJoinRetryTimer, &QTimer::timeout, this, &MumbleClientEngine::autoJoinResolvedChannel);
}

MumbleClientEngine::~MumbleClientEngine() {
    disconnectFromServer(true);
}

void MumbleClientEngine::connectToServer(const QString &host, int port, const QString &username, const QString &password) {
    m_lastHost = host;
    m_lastPort = port;
    m_baseUsername = username.isEmpty() ? "Ken_Narottama" : username;
    m_currentUsername = m_baseUsername;
    m_serverPassword = password;
    m_suffixIndex = 0;
    m_isIntentionalDisconnect = false;
    m_autoJoinPending = true;
    m_autoJoinRetryCount = 0;

    m_channels.clear();
    m_users.clear();
    m_mySessionId = -1;
    m_readBuffer.clear();

    emit logEmitted(QString("[MUMBLE ENGINE] Connecting to %1:%2 as '%3'...").arg(host).arg(port).arg(m_currentUsername));
    m_socket->abort();
    m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    m_socket->connectToHostEncrypted(host, static_cast<quint16>(port));
}

void MumbleClientEngine::disconnectFromServer(bool intentional) {
    m_isIntentionalDisconnect = intentional;
    m_pingTimer->stop();
    m_talkingResetTimer->stop();
    m_autoJoinRetryTimer->stop();
    m_audioEngine.stopAudio();

    if (intentional) {
        m_lastJoinedChannelId = -1;
        m_autoJoinPending = false;
    }

    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnectFromHost();
        m_socket->close();
    }

    m_isConnected = false;
    m_mySessionId = -1;
    m_channels.clear();
    m_users.clear();

    emit connectionStateChanged(false);
    emit stateUpdated();
}

void MumbleClientEngine::onSocketConnected() {
    m_isConnected = true;
    m_reconnectAttempts = 0;
    m_autoJoinPending = true;
    m_autoJoinRetryCount = 0;
    m_audioEngine.startAudio();
    emit logEmitted("[MUMBLE ENGINE] SSL Handshake Successful. Initializing Handshake...");

    // 1. Send Packet Type 0 (Version)
    MumbleBufferWriter versionWriter;
    versionWriter.writeVarintField(1, (1 << 16) | (4 << 8) | 0); // Version 1.4.0
    versionWriter.writeStringField(2, "SDC YAJB 1.4.0");
    versionWriter.writeStringField(3, "Windows");
    versionWriter.writeStringField(4, "SDC YAJB Desktop");
    sendControlPacket(0, versionWriter.toByteArray());

    // 2. Send Packet Type 2 (Authenticate)
    MumbleBufferWriter authWriter;
    authWriter.writeStringField(1, m_currentUsername);
    authWriter.writeStringField(2, m_serverPassword);

    QSet<QString> tokenSet;
    for (const QString &t : m_accessTokens) {
        QStringList subTokens = t.split(",", Qt::SkipEmptyParts);
        for (const QString &st : subTokens) {
            QString trimmed = st.trimmed();
            if (!trimmed.isEmpty()) {
                tokenSet.insert(trimmed);
            }
        }
    }
    for (const QString &tok : tokenSet) {
        authWriter.writeStringField(3, tok); // Field 3 in Authenticate protobuf is repeated string tokens = 3
    }
    authWriter.writeVarintField(5, 1); // Opus support = true
    sendControlPacket(2, authWriter.toByteArray());

    m_pingTimer->start(5000);
    m_talkingResetTimer->start(400);
    emit connectionStateChanged(true);
}

void MumbleClientEngine::onSocketReadyRead() {
    m_readBuffer.append(m_socket->readAll());

    while (m_readBuffer.size() >= 6) {
        const char *data = m_readBuffer.constData();
        quint16 type = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
        quint32 len = (static_cast<quint8>(data[2]) << 24) |
                      (static_cast<quint8>(data[3]) << 16) |
                      (static_cast<quint8>(data[4]) << 8) |
                      static_cast<quint8>(data[5]);

        if (m_readBuffer.size() < static_cast<int>(6 + len)) {
            break; // Wait for full packet payload
        }

        QByteArray payload = m_readBuffer.mid(6, len);
        m_readBuffer.remove(0, 6 + len);

        if (type == 1) {
            processAudioPacket(payload);
        } else {
            processControlPacket(type, payload);
        }
    }
}

void MumbleClientEngine::onSocketError(QAbstractSocket::SocketError error) {
    QString errStr = m_socket->errorString();
    emit logEmitted(QString("[SOCKET ERROR %1] %2").arg(error).arg(errStr));

    if (m_isConnected) {
        emit connectionError(QString("Terputus dari server: %1").arg(errStr));
    }

    if (!m_isIntentionalDisconnect) {
        m_isConnected = false;
        emit connectionStateChanged(false);

        if (m_reconnectAttempts < 5) {
            m_reconnectAttempts++;
            int delayMs = 2000 * m_reconnectAttempts;
            emit logEmitted(QString("[AUTO RECONNECT] Reconnecting attempt %1 in %2ms...").arg(m_reconnectAttempts).arg(delayMs));
            QTimer::singleShot(delayMs, this, [this]() {
                if (!m_isIntentionalDisconnect && !m_isConnected) {
                    m_socket->abort();
                    m_readBuffer.clear();
                    m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
                    m_socket->connectToHostEncrypted(m_lastHost, static_cast<quint16>(m_lastPort));
                }
            });
        }
    }
}

void MumbleClientEngine::processControlPacket(quint16 type, const QByteArray &payload) {
    switch (type) {
    case 5: // Reject
        parseReject(payload);
        break;
    case 7: // ChannelState
        parseChannelState(payload);
        break;
    case 8: // ChannelRemove
        parseChannelRemove(payload);
        break;
    case 9: // UserState
        parseUserState(payload);
        break;
    case 10: // UserRemove
        parseUserRemove(payload);
        break;
    case 11: // ServerSync
        parseServerSync(payload);
        break;
    case 12: // PermissionDenied
        parsePermissionDenied(payload);
        break;
    default:
        break;
    }
}

void MumbleClientEngine::processAudioPacket(const QByteArray &payload) {
    if (payload.size() < 4) return;

    quint8 headerByte = static_cast<quint8>(payload[0]);
    quint8 audioType = (headerByte >> 5) & 0x07;
    if (audioType != 4) return; // 4 = Opus

    int offset = 1;
    quint64 session = readMumbleVarint(payload, offset);
    quint64 seq = readMumbleVarint(payload, offset);
    Q_UNUSED(seq);

    quint64 opusHeader = readMumbleVarint(payload, offset);
    bool isTerminator = (opusHeader & 0x2000) != 0;
    int opusLen = static_cast<int>(opusHeader & 0x1FFF);

    if (opusLen > 0 && offset + opusLen <= payload.size()) {
        const unsigned char *dataPtr = reinterpret_cast<const unsigned char *>(payload.constData()) + offset;
        m_audioEngine.decodeOpusToSpeaker(dataPtr, opusLen);
    }

    int sid = static_cast<int>(session);
    if (m_users.contains(sid)) {
        m_users[sid].isTalking = !isTerminator;
        emit stateUpdated();
    }
}

void MumbleClientEngine::parseReject(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    quint64 rejectType = 0;
    QString reason;

    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);

        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) rejectType = val;
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            if (fieldNum == 2) reason = reader.readString(len);
            else reader.skip(len);
        } else if (wireType == 1) reader.skip(8);
        else if (wireType == 5) reader.skip(4);
    }

    emit logEmitted(QString("[REJECT] Type: %1, Reason: %2").arg(rejectType).arg(reason));

    if (rejectType == 1 || rejectType == 2 || rejectType == 3 ||
        reason.contains("name", Qt::CaseInsensitive) ||
        reason.contains("user", Qt::CaseInsensitive) ||
        reason.contains("in use", Qt::CaseInsensitive) ||
        reason.contains("terpakai", Qt::CaseInsensitive) ||
        reason.contains("already", Qt::CaseInsensitive) ||
        reason.contains("invalid", Qt::CaseInsensitive)) {
        
        if (m_suffixIndex < 10) {
            m_suffixIndex++;
            m_currentUsername = QString("%1_%2").arg(m_baseUsername).arg(m_suffixIndex, 2, 10, QChar('0'));
            emit logEmitted(QString("[SUFFIX ROTATION] Username sudah ada di server. Mencoba '%1'...").arg(m_currentUsername));
            QTimer::singleShot(400, [this]() {
                m_socket->abort();
                m_readBuffer.clear();
                m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
                m_socket->connectToHostEncrypted(m_lastHost, static_cast<quint16>(m_lastPort));
            });
        }
    }
}

void MumbleClientEngine::parseChannelState(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    int channelId = -1;
    int parentId = -1;
    QString name;
    int position = 0;
    bool hasParent = false;
    bool hasName = false;

    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);

        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) channelId = static_cast<int>(val);
            else if (fieldNum == 2) { parentId = static_cast<int>(val); hasParent = true; }
            else if (fieldNum == 5) position = static_cast<int>(val);
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            if (fieldNum == 3) { name = reader.readString(len); hasName = true; }
            else reader.skip(len);
        } else if (wireType == 1) reader.skip(8);
        else if (wireType == 5) reader.skip(4);
    }

    if (channelId >= 0) {
        if (!m_channels.contains(channelId)) {
            MumbleChannel c;
            c.id = channelId;
            c.parentId = hasParent ? parentId : -1;
            c.name = hasName ? name : QString("Channel %1").arg(channelId);
            c.position = position;
            c.isExpanded = false;
            m_channels[channelId] = c;
        } else {
            if (hasParent) m_channels[channelId].parentId = parentId;
            if (hasName) m_channels[channelId].name = name;
            m_channels[channelId].position = position;
        }

        if (m_autoJoinPending && m_mySessionId >= 0) {
            m_autoJoinRetryTimer->start(200);
        }

        emit stateUpdated();
    }
}

void MumbleClientEngine::parseChannelRemove(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    int channelId = -1;
    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);
        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) channelId = static_cast<int>(val);
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            reader.skip(len);
        }
    }
    if (channelId >= 0 && m_channels.contains(channelId)) {
        m_channels.remove(channelId);
        emit stateUpdated();
    }
}

void MumbleClientEngine::parseUserState(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    int session = 0;
    int channelId = -1;
    QString name;
    bool hasChannel = false;
    bool hasName = false;
    bool isSelfMute = false;
    bool isSelfDeaf = false;

    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);

        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) session = static_cast<int>(val);
            else if (fieldNum == 5) { channelId = static_cast<int>(val); hasChannel = true; }
            else if (fieldNum == 7) isSelfMute = (val != 0);
            else if (fieldNum == 8) isSelfDeaf = (val != 0);
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            if (fieldNum == 3) { name = reader.readString(len); hasName = true; }
            else reader.skip(len);
        } else if (wireType == 1) reader.skip(8);
        else if (wireType == 5) reader.skip(4);
    }

    if (session > 0) {
        if (!m_users.contains(session)) {
            MumbleUser u;
            u.session = session;
            u.channelId = hasChannel ? channelId : 0;
            u.name = hasName ? name : QString("User %1").arg(session);
            u.isMuted = isSelfMute;
            u.isDeafened = isSelfDeaf;
            u.isSelf = (session == m_mySessionId);
            m_users[session] = u;
        } else {
            if (hasChannel) m_users[session].channelId = channelId;
            if (hasName) m_users[session].name = name;
            m_users[session].isMuted = isSelfMute;
            m_users[session].isDeafened = isSelfDeaf;
            if (session == m_mySessionId) m_users[session].isSelf = true;
        }

        if (session == m_mySessionId && hasChannel) {
            m_lastJoinedChannelId = channelId;
            collapseAllExceptUserAncestors(channelId);
        }

        emit stateUpdated();
    }
}

void MumbleClientEngine::parseUserRemove(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    int session = -1;
    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);
        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) session = static_cast<int>(val);
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            reader.skip(len);
        }
    }
    if (session >= 0 && m_users.contains(session)) {
        m_users.remove(session);
        emit stateUpdated();
    }
}

void MumbleClientEngine::parseServerSync(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);

        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) {
                m_mySessionId = static_cast<int>(val);
                if (m_users.contains(m_mySessionId)) {
                    m_users[m_mySessionId].isSelf = true;
                }
            }
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            reader.skip(len);
        } else if (wireType == 1) reader.skip(8);
        else if (wireType == 5) reader.skip(4);
    }

    emit logEmitted(QString("[MUMBLE ENGINE] ServerSync Received. My Session ID = %1").arg(m_mySessionId));

    autoJoinResolvedChannel();
    m_autoJoinRetryTimer->start(300);

    emit stateUpdated();
}

void MumbleClientEngine::parsePermissionDenied(const QByteArray &payload) {
    MumbleBufferReader reader(payload);
    quint64 denyType = 0;
    QString reason;
    quint64 channelId = 0;

    while (reader.hasMore()) {
        quint64 tag = reader.readVarint();
        int fieldNum = static_cast<int>(tag >> 3);
        int wireType = static_cast<int>(tag & 0x07);

        if (wireType == 0) {
            quint64 val = reader.readVarint();
            if (fieldNum == 1) channelId = val;
            else if (fieldNum == 3) denyType = val;
        } else if (wireType == 2) {
            int len = static_cast<int>(reader.readVarint());
            if (fieldNum == 2) reason = reader.readString(len);
            else reader.skip(len);
        } else if (wireType == 1) reader.skip(8);
        else if (wireType == 5) reader.skip(4);
    }

    QString denyMsg = reason.isEmpty()
        ? QString("Akses ditolak pada channel %1 (Tipe: %2). Periksa Access Token Anda.").arg(channelId).arg(denyType)
        : QString("Akses Ditolak: %1").arg(reason);

    emit logEmitted(QString("[PERMISSION DENIED] %1").arg(denyMsg));
    emit permissionDeniedOccurred(denyMsg);
}

void MumbleClientEngine::moveToChannel(int targetChannelId) {
    if (!m_isConnected) return;
    m_lastJoinedChannelId = targetChannelId;

    emit logEmitted(QString("[ENGINE] moveToChannel -> Mengirim UserState Type 9 (Session: %1, Channel: %2)").arg(m_mySessionId).arg(targetChannelId));

    MumbleBufferWriter userWriter;
    if (m_mySessionId >= 0) {
        userWriter.writeVarintField(1, static_cast<quint64>(m_mySessionId));
    }
    userWriter.writeVarintField(5, static_cast<quint64>(targetChannelId));
    sendControlPacket(9, userWriter.toByteArray()); // Type 9 = UserState
}

void MumbleClientEngine::setChannelExpanded(int channelId, bool expanded) {
    if (m_channels.contains(channelId)) {
        m_channels[channelId].isExpanded = expanded;
        emit stateUpdated();
    }
}

void MumbleClientEngine::setPttActive(bool active) {
    if (m_isPttActive == active) return;
    m_isPttActive = active;
    m_audioEngine.setPttActive(active);

    if (!active) {
        // Send final terminator frame (bit 13 = 0x2000)
        QByteArray audioPayload;
        audioPayload.append(static_cast<char>(0x80)); // 0x80 = Opus audio to current channel
        m_audioSequence++;
        writeMumbleVarint(audioPayload, m_audioSequence);
        writeMumbleVarint(audioPayload, 0x2000);
        sendControlPacket(1, audioPayload);
    }

    if (m_mySessionId >= 0 && m_users.contains(m_mySessionId)) {
        m_users[m_mySessionId].isTalking = active;
        emit stateUpdated();
    }
}

void MumbleClientEngine::onOpusFrameReady(const QByteArray &opusBytes) {
    if (!m_isConnected || !m_isPttActive || opusBytes.isEmpty()) return;

    QByteArray audioPayload;
    audioPayload.append(static_cast<char>(0x80)); // 0x80 = Opus audio to current channel

    m_audioSequence++;
    writeMumbleVarint(audioPayload, m_audioSequence);

    quint64 opusHeader = static_cast<quint64>(opusBytes.size());
    writeMumbleVarint(audioPayload, opusHeader);
    audioPayload.append(opusBytes);

    sendControlPacket(1, audioPayload); // Type 1 = UDPTunnel
}

void MumbleClientEngine::sendAccessTokens() {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;

    MumbleBufferWriter authWriter;
    QSet<QString> tokenSet;
    for (const QString &t : m_accessTokens) {
        QStringList subTokens = t.split(",", Qt::SkipEmptyParts);
        for (const QString &st : subTokens) {
            QString trimmed = st.trimmed();
            if (!trimmed.isEmpty()) {
                tokenSet.insert(trimmed);
            }
        }
    }

    for (const QString &tok : tokenSet) {
        authWriter.writeStringField(3, tok); // Field 3 = repeated string tokens
    }
    sendControlPacket(2, authWriter.toByteArray()); // Type 2 = Authenticate
}

void MumbleClientEngine::collapseAllExceptUserAncestors(int targetChannelId) {
    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
        it.value().isExpanded = false;
    }
    int curr = targetChannelId;
    int safety = 0;
    while (m_channels.contains(curr) && safety++ < 30) {
        m_channels[curr].isExpanded = true;
        int pid = m_channels[curr].parentId;
        if (pid < 0 || pid == curr) break;
        curr = pid;
    }
    emit stateUpdated();
}

void MumbleClientEngine::autoJoinResolvedChannel() {
    if (!m_isConnected || m_mySessionId < 0) return;

    // Check current user channel position
    int currentChannelId = -1;
    if (m_users.contains(m_mySessionId)) {
        currentChannelId = m_users[m_mySessionId].channelId;
    }

    // If reconnecting after unintentional drop and we already had a joined channel, use that
    if (!m_isIntentionalDisconnect && m_lastJoinedChannelId >= 0 && m_channels.contains(m_lastJoinedChannelId)) {
        if (currentChannelId == m_lastJoinedChannelId) {
            m_autoJoinPending = false;
            m_autoJoinRetryTimer->stop();
            collapseAllExceptUserAncestors(m_lastJoinedChannelId);
            return;
        }
        emit logEmitted(QString("[AUTO JOIN] Mengembalikan ke channel terakhir (ID: %1).").arg(m_lastJoinedChannelId));
        moveToChannel(m_lastJoinedChannelId);
        collapseAllExceptUserAncestors(m_lastJoinedChannelId);
        m_autoJoinRetryCount++;
        if (m_autoJoinRetryCount < 15) {
            m_autoJoinRetryTimer->start(350);
        } else {
            m_autoJoinPending = false;
        }
        return;
    }

    QString rawTarget = m_autoJoinChannelName.trimmed();
    if (rawTarget.isEmpty() || rawTarget == "\\" || rawTarget == "/") {
        if (currentChannelId == 0) {
            m_autoJoinPending = false;
            m_autoJoinRetryTimer->stop();
            collapseAllExceptUserAncestors(0);
            return;
        }
        emit logEmitted(QString("[AUTO JOIN] Target root channel (ID: 0)."));
        moveToChannel(0);
        collapseAllExceptUserAncestors(0);
        m_autoJoinRetryCount++;
        if (m_autoJoinRetryCount < 15) {
            m_autoJoinRetryTimer->start(350);
        } else {
            m_autoJoinPending = false;
        }
        return;
    }

    QString targetNorm = rawTarget;
    targetNorm.replace("\\", "/");
    while (targetNorm.startsWith("/")) targetNorm.remove(0, 1);
    while (targetNorm.endsWith("/")) targetNorm.chop(1);

    QString targetLeaf = targetNorm.split("/").last().trimmed();
    int matchedChannelId = -1;

    // Pass 1: Match full path suffix
    for (const auto &chan : m_channels) {
        QStringList pathParts;
        int currId = chan.id;
        int safety = 0;
        while (m_channels.contains(currId) && safety++ < 20) {
            pathParts.prepend(m_channels[currId].name.trimmed());
            int pid = m_channels[currId].parentId;
            if (pid < 0 || pid == currId) break;
            currId = pid;
        }
        QString fullPath = pathParts.join("/");
        if (fullPath.compare(targetNorm, Qt::CaseInsensitive) == 0 ||
            fullPath.endsWith("/" + targetNorm, Qt::CaseInsensitive) ||
            fullPath.endsWith(targetNorm, Qt::CaseInsensitive)) {
            matchedChannelId = chan.id;
            break;
        }
    }

    // Pass 2: Match leaf name exactly
    if (matchedChannelId < 0 && !targetLeaf.isEmpty()) {
        for (const auto &chan : m_channels) {
            if (chan.name.trimmed().compare(targetLeaf, Qt::CaseInsensitive) == 0) {
                matchedChannelId = chan.id;
                break;
            }
        }
    }

    // Pass 3: Match leaf substring
    if (matchedChannelId < 0 && !targetLeaf.isEmpty()) {
        for (const auto &chan : m_channels) {
            if (chan.name.contains(targetLeaf, Qt::CaseInsensitive)) {
                matchedChannelId = chan.id;
                break;
            }
        }
    }

    if (matchedChannelId >= 0) {
        if (currentChannelId == matchedChannelId) {
            emit logEmitted(QString("[AUTO JOIN CONFIRMED] User sekarang berada di channel target '%1' (ID: %2).").arg(m_autoJoinChannelName).arg(matchedChannelId));
            m_autoJoinPending = false;
            m_autoJoinRetryTimer->stop();
            collapseAllExceptUserAncestors(matchedChannelId);
            return;
        }

        emit logEmitted(QString("[AUTO JOIN ATTEMPT %1] Menemukan target channel '%2' (ID: %3). Mengirim pemindahan channel...").arg(m_autoJoinRetryCount + 1).arg(m_autoJoinChannelName).arg(matchedChannelId));
        moveToChannel(matchedChannelId);
        collapseAllExceptUserAncestors(matchedChannelId);

        m_autoJoinRetryCount++;
        if (m_autoJoinRetryCount < 15) {
            m_autoJoinRetryTimer->start(350);
        } else {
            m_autoJoinPending = false;
        }
    } else {
        m_autoJoinRetryCount++;
        emit logEmitted(QString("[AUTO JOIN PENDING] Channel '%1' belum ditemukan (%2 channel di memori, retry %3/15).").arg(m_autoJoinChannelName).arg(m_channels.size()).arg(m_autoJoinRetryCount));
        if (m_autoJoinRetryCount < 15) {
            m_autoJoinRetryTimer->start(350);
        } else {
            m_autoJoinPending = false;
        }
    }
}

void MumbleClientEngine::sendControlPacket(quint16 type, const QByteArray &payload) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray packet;
    packet.append(static_cast<char>((type >> 8) & 0xFF));
    packet.append(static_cast<char>(type & 0xFF));

    quint32 len = static_cast<quint32>(payload.size());
    packet.append(static_cast<char>((len >> 24) & 0xFF));
    packet.append(static_cast<char>((len >> 16) & 0xFF));
    packet.append(static_cast<char>((len >> 8) & 0xFF));
    packet.append(static_cast<char>(len & 0xFF));

    packet.append(payload);
    m_socket->write(packet);
}
