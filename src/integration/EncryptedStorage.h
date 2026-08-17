#ifndef ENCRYPTEDSTORAGE_H
#define ENCRYPTEDSTORAGE_H

#include <QString>
#include <QJsonObject>

class EncryptedStorage {
public:
    static bool saveSecureString(const QString &key, const QString &value);
    static QString readSecureString(const QString &key);
    static void clearSecureKey(const QString &key);

    static bool saveUserProfile(const QJsonObject &profileObj);
    static QJsonObject loadUserProfile();
};

#endif // ENCRYPTEDSTORAGE_H
