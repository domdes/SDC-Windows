#include "EncryptedStorage.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

static QString getStorageFilePath(const QString &fileName) {
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath(fileName);
}

bool EncryptedStorage::saveSecureString(const QString &key, const QString &value) {
#ifdef Q_OS_WIN
    QByteArray plainData = value.toUtf8();
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    dataIn.pbData = reinterpret_cast<BYTE*>(plainData.data());
    dataIn.cbData = static_cast<DWORD>(plainData.size());

    if (CryptProtectData(&dataIn, L"SDCYAJBSecureData", nullptr, nullptr, nullptr, 0, &dataOut)) {
        QByteArray encryptedData(reinterpret_cast<char*>(dataOut.pbData), static_cast<int>(dataOut.cbData));
        LocalFree(dataOut.pbData);

        QFile file(getStorageFilePath(key + ".dat"));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(encryptedData);
            file.close();
            return true;
        }
    }
    return false;
#else
    QFile file(getStorageFilePath(key + ".dat"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(value.toUtf8().toBase64());
        file.close();
        return true;
    }
    return false;
#endif
}

QString EncryptedStorage::readSecureString(const QString &key) {
    QFile file(getStorageFilePath(key + ".dat"));
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QByteArray encryptedData = file.readAll();
    file.close();

#ifdef Q_OS_WIN
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    dataIn.pbData = reinterpret_cast<BYTE*>(encryptedData.data());
    dataIn.cbData = static_cast<DWORD>(encryptedData.size());

    if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
        QByteArray decryptedData(reinterpret_cast<char*>(dataOut.pbData), static_cast<int>(dataOut.cbData));
        LocalFree(dataOut.pbData);
        return QString::fromUtf8(decryptedData);
    }
    return QString();
#else
    return QString::fromUtf8(QByteArray::fromBase64(encryptedData));
#endif
}

void EncryptedStorage::clearSecureKey(const QString &key) {
    QFile::remove(getStorageFilePath(key + ".dat"));
}

bool EncryptedStorage::saveUserProfile(const QJsonObject &profileObj) {
    QJsonDocument doc(profileObj);
    return saveSecureString("user_profile", doc.toJson(QJsonDocument::Compact));
}

QJsonObject EncryptedStorage::loadUserProfile() {
    QString jsonStr = readSecureString("user_profile");
    if (jsonStr.isEmpty()) return QJsonObject();

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    return doc.object();
}
