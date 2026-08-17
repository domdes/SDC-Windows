#ifndef PROFILEVIEWMODEL_H
#define PROFILEVIEWMODEL_H
#include <QObject>
class ProfileViewModel : public QObject {
    Q_OBJECT
public:
    explicit ProfileViewModel(QObject *parent = nullptr) : QObject(parent) {}
};
#endif
