#ifndef LOGINVIEWMODEL_H
#define LOGINVIEWMODEL_H
#include <QObject>
class LoginViewModel : public QObject {
    Q_OBJECT
public:
    explicit LoginViewModel(QObject *parent = nullptr) : QObject(parent) {}
};
#endif
