#include "AppViewModel.h"
#include "EncryptedStorage.h"
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

AppViewModel::AppViewModel(QObject *parent)
    : QObject(parent) {
    m_voiceViewModel = new VoiceViewModel(&m_mumbleEngine, this);

    connect(&m_oauthService, &OAuthService::loginSuccess, this, [this](const QString &accessToken, const QString &refreshToken, const QString &userEmail) {
        Q_UNUSED(refreshToken);
        qDebug() << "[APP VIEWMODEL] Login Google success for email:" << userEmail;
        m_activeUserEmail = userEmail;
        EncryptedStorage::saveSecureString("user_email", userEmail);
        EncryptedStorage::saveSecureString("access_token", accessToken);
        m_portalService.fetchUserProfile(userEmail);
        bringWindowToFront();
    });

    connect(&m_oauthService, &OAuthService::loginFailed, this, [this](const QString &errorMsg) {
        qWarning() << "[APP VIEWMODEL] OAuth Login failed:" << errorMsg;
    });

    connect(&m_portalService, &PortalApiService::profileFetched, this, [this](const UserProfileData &data) {
        m_activeMumbleUsername = data.username.trimmed();
        m_activeUserRole = data.role.trimmed().toLower();
        m_activeUserName = data.name.trimmed();
        m_activeUserEmail = data.email.trimmed();

        if (m_voiceViewModel) {
            m_voiceViewModel->setUserRole(m_activeUserRole);
        }

        qDebug() << "[APP VIEWMODEL] Logged in user from whitelist:" << m_activeMumbleUsername << "Role:" << m_activeUserRole;

        m_currentView = "ProfileListView";
        emit currentViewChanged();
        emit activeUserChanged();
    });

    connect(&m_portalService, &PortalApiService::profilesFetched, this, &AppViewModel::populateProfiles);

    autoLoginOrShowLogin();
}

void AppViewModel::autoLoginOrShowLogin() {
    QString savedEmail = EncryptedStorage::readSecureString("user_email");
    if (savedEmail.trimmed().isEmpty()) {
        m_currentView = "LoginView";
        emit currentViewChanged();
        return;
    }
    m_activeUserEmail = savedEmail.trimmed();
    m_portalService.fetchUserProfile(savedEmail.trimmed());
}

void AppViewModel::populateProfiles(const QList<MumbleProfileData> &profiles) {
    m_rawProfiles = profiles;
    m_preinstalledProfiles.clear();
    m_savedProfiles.clear();

    for (const auto &p : profiles) {
        if (!p.isPreinstalled) continue; // STRICT FILTER: ONLY is_preinstalled == true

        QVariantMap map;
        map["id"] = p.id;
        map["name"] = p.name;
        map["host"] = p.host;
        map["port"] = p.port;
        map["password"] = p.password.isEmpty() ? "4622bekasiselatan" : p.password;
        map["guruChannel"] = p.guruChannel;
        map["muridChannel"] = p.muridChannel;
        map["guruAccessToken"] = p.guruAccessToken;
        map["muridAccessToken"] = p.muridAccessToken;
        map["isPreinstalled"] = true;

        m_preinstalledProfiles.append(map);
    }

    // Fallback default preinstalled if database query returns empty
    if (m_preinstalledProfiles.isEmpty()) {
        QVariantMap p1;
        p1["id"] = "cf1a1f87-948c-412d-9201-5f5466327c9a";
        p1["name"] = "Daerah Bekasi Selatan";
        p1["host"] = "993.kanzul-mubaarok.org";
        p1["port"] = 64738;
        p1["password"] = "4622bekasiselatan";
        p1["guruChannel"] = "\\";
        p1["muridChannel"] = "\\";
        p1["guruAccessToken"] = "";
        p1["muridAccessToken"] = "";
        p1["isPreinstalled"] = true;
        m_preinstalledProfiles.append(p1);

        QVariantMap p2;
        p2["id"] = "1f6c99e2-6e9f-417f-a104-38bb4a6897c0";
        p2["name"] = "Desa Cikunir";
        p2["host"] = "asrama.kanzul-mubaarok.org";
        p2["port"] = 51208;
        p2["password"] = "4622bekasiselatan";
        p2["guruChannel"] = "Cikunir\\Pengajian Desa";
        p2["muridChannel"] = "Cikunir\\Pengajian Desa";
        p2["guruAccessToken"] = "ustadz,354";
        p2["muridAccessToken"] = "";
        p2["isPreinstalled"] = true;
        m_preinstalledProfiles.append(p2);
    }

    emit profilesChanged();
}

void AppViewModel::startGoogleLogin() {
    m_oauthService.startGoogleLogin();
}

void AppViewModel::handleUriScheme(const QString &uri) {
    qDebug() << "[APP VIEWMODEL] Handling URI scheme:" << uri;
    QUrl url(uri);
    QUrlQuery query(url);
    
    QString accessToken = query.queryItemValue("access_token");
    QString refreshToken = query.queryItemValue("refresh_token");
    QString code = query.queryItemValue("code");

    if (!accessToken.isEmpty()) {
        m_oauthService.fetchUserEmail(accessToken, refreshToken);
    bringWindowToFront();
    } else if (!code.isEmpty()) {
        m_oauthService.exchangeAuthorizationCode(code);
    }
}

void AppViewModel::connectProfileById(const QString &profileId) {
    MumbleProfileData selectedProfile;
    bool found = false;

    for (const auto &p : m_rawProfiles) {
        if (p.id == profileId || p.host == profileId || p.name == profileId) {
            selectedProfile = p;
            found = true;
            break;
        }
    }

    if (!found) {
        for (const auto &var : m_preinstalledProfiles) {
            QVariantMap map = var.toMap();
            if (map["id"].toString() == profileId || map["host"].toString() == profileId) {
                selectedProfile.id = map["id"].toString();
                selectedProfile.name = map["name"].toString();
                selectedProfile.host = map["host"].toString();
                selectedProfile.port = map["port"].toInt();
                selectedProfile.password = map["password"].toString();
                selectedProfile.guruChannel = map["guruChannel"].toString();
                selectedProfile.muridChannel = map["muridChannel"].toString();
                selectedProfile.guruAccessToken = map["guruAccessToken"].toString();
                selectedProfile.muridAccessToken = map["muridAccessToken"].toString();
                found = true;
                break;
            }
        }
    }

    if (!found) {
        selectedProfile.host = "993.kanzul-mubaarok.org";
        selectedProfile.port = 64738;
        selectedProfile.password = "4622bekasiselatan";
        selectedProfile.guruChannel = "\\";
        selectedProfile.muridChannel = "\\";
    }

    if (selectedProfile.password.trimmed().isEmpty()) {
        selectedProfile.password = "4622bekasiselatan";
    }

    // Determine target channel and access token according to active user role
    QString targetChannel;
    QString targetToken;
    if (m_activeUserRole.contains("guru", Qt::CaseInsensitive) || m_activeUserRole.contains("admin", Qt::CaseInsensitive)) {
        targetChannel = selectedProfile.guruChannel;
        targetToken = selectedProfile.guruAccessToken;
    } else {
        targetChannel = selectedProfile.muridChannel;
        targetToken = selectedProfile.muridAccessToken;
    }

    // 1. Read custom tokens from settings and merge with profile tokens (trimmed without spaces)
    QString savedTokens = EncryptedStorage::readSecureString("custom_tokens");
    if (savedTokens.trimmed().isEmpty()) {
        savedTokens = "ustadz,relay,00_ 002_,guru,murid,cikunir,asrama,desa,354";
    }
    QStringList rawList = savedTokens.split(",", Qt::SkipEmptyParts);
    QStringList tokenList;
    for (const QString &t : rawList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !tokenList.contains(trimmed, Qt::CaseInsensitive)) {
            tokenList.append(trimmed);
        }
    }

    if (!targetToken.trimmed().isEmpty()) {
        QStringList pTokens = targetToken.split(",", Qt::SkipEmptyParts);
        for (const QString &t : pTokens) {
            QString trimmed = t.trimmed();
            if (!trimmed.isEmpty() && !tokenList.contains(trimmed, Qt::CaseInsensitive)) {
                tokenList.append(trimmed);
            }
        }
    }
    m_voiceViewModel->updateAccessTokens(tokenList.join(","));

    // 2. Set auto-join channel for the selected profile
    qDebug() << "[CONNECT PROFILE]" << selectedProfile.name << "Host:" << selectedProfile.host << "AutoJoin Channel:" << targetChannel;
    m_mumbleEngine.setAutoJoinChannelName(targetChannel);

    // 3. Connect to Mumble with whitelist username from mumble_users table
    QString finalMumbleUser = m_activeMumbleUsername.trimmed();
    if (finalMumbleUser.isEmpty()) {
        finalMumbleUser = "12.Jabar_08.Bekasi.Selatan_CIKUNIR.5_Ken.WEB";
    }

    m_voiceViewModel->connectToServer(selectedProfile.host, selectedProfile.port, finalMumbleUser, selectedProfile.password);
    m_currentView = "ConnectedView";
    emit currentViewChanged();
}

void AppViewModel::connectProfile(const QString &host, int port) {
    connectProfileById(host);
}

void AppViewModel::disconnectAndReturnToProfiles() {
    if (m_voiceViewModel) {
        m_voiceViewModel->disconnect();
    }
    m_currentView = "ProfileListView";
    emit currentViewChanged();
}

void AppViewModel::logout() {
    disconnectAndReturnToProfiles();
    EncryptedStorage::saveSecureString("user_email", "");
    m_activeUserEmail = "";
    m_activeUserName = "";
    m_activeMumbleUsername = "";
    m_activeUserRole = "";
    m_currentView = "LoginView";
    emit currentViewChanged();
    emit activeUserChanged();
}

#ifdef Q_OS_WIN
#include <windows.h>
static void forceForegroundWindow(HWND hwnd) {
    if (!hwnd) return;
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    if (currentThreadId != foregroundThreadId) {
        AttachThreadInput(currentThreadId, foregroundThreadId, TRUE);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(currentThreadId, foregroundThreadId, FALSE);
    } else {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, SW_SHOW);
}

void AppViewModel::bringWindowToFront() {
    HWND hwnd = FindWindowW(NULL, L"SDC YAJB - Yayasan Asyuhada Jaya Bekasi");
    if (hwnd) {
        forceForegroundWindow(hwnd);
    }
}
#else
void AppViewModel::bringWindowToFront() {}
#endif
