#include "PortalApiService.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

static const char *SupabaseUrl = "https://faythocihlyzmzrnwwks.supabase.co/rest/v1";
static const char *SupabaseAnonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImZheXRob2NpaGx5em16cm53d2tzIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzYxNTYyOTcsImV4cCI6MjA5MTczMjI5N30.NurwRqdxe_oqnb8dNBVk8fT0P4vFxZBo1yimvw23Bws";

PortalApiService::PortalApiService(QObject *parent) : QObject(parent) {}

void PortalApiService::fetchUserProfile(const QString &email) {
    m_currentData.email = email.isEmpty() ? "ken.narottama@gmail.com" : email.trimmed();

    QUrl url(QString("%1/mumble_users?select=*&email=eq.%2&deleted_at=is.null").arg(SupabaseUrl, m_currentData.email));
    QNetworkRequest request(url);
    request.setRawHeader("apikey", SupabaseAnonKey);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(SupabaseAnonKey).toUtf8());

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
            if (!arr.isEmpty()) {
                QJsonObject obj = arr.first().toObject();
                m_currentData.username = obj["username"].toString().trimmed();
                m_currentData.role = obj["role"].toString("guru").trimmed();
                m_currentData.name = m_currentData.username;
                m_currentData.desa = "Cikunir";
                qDebug() << "[PORTAL API] Loaded Whitelist User:" << m_currentData.username << "Role:" << m_currentData.role;
            }
        }
        emit profileFetched(m_currentData);
        fetchAllMumbleConfigs();
    });
}

void PortalApiService::fetchAllMumbleConfigs() {
    // Only fetch configs from mumble_config where is_preinstalled is true
    QUrl url(QString("%1/mumble_config?select=*&is_preinstalled=eq.true").arg(SupabaseUrl));
    QNetworkRequest request(url);
    request.setRawHeader("apikey", SupabaseAnonKey);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(SupabaseAnonKey).toUtf8());

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
            m_profilesList.clear();

            for (const QJsonValue &val : arr) {
                QJsonObject obj = val.toObject();
                MumbleProfileData p;
                p.id = obj["id"].toString();
                p.name = obj["profile_name"].toString("Server SDC");
                p.host = obj["mumble_host"].toString();
                p.port = obj["mumble_port"].toInt(64738);
                p.password = obj["mumble_password"].toString("4622bekasiselatan");
                p.isPreinstalled = obj["is_preinstalled"].toBool(false);
                p.guruChannel = obj["guru_channel"].toString();
                p.muridChannel = obj["murid_channel"].toString();
                p.guruAccessToken = obj["guru_access_token"].toString();
                p.muridAccessToken = obj["murid_access_token"].toString();

                if (p.isPreinstalled) {
                    m_profilesList.append(p);
                }
            }
            qDebug() << "[PORTAL API] Loaded" << m_profilesList.size() << "Pre-installed Mumble connection profiles.";
            emit profilesFetched(m_profilesList);
        }
    });
}
