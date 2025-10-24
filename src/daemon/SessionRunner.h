#ifndef PLASMALOGIN_SESSIONRUNNER_H
#define PLASMALOGIN_SESSIONRUNNER_H

#include <QObject>
#include <QString>
#include <QProcessEnvironment>

namespace PLASMALOGIN
{
class SessionRunner : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SessionRunner)
public:
    explicit SessionRunner(QObject *parent = nullptr);
    ~SessionRunner();

    void setUser(const QString &user);
    void setGreeter(bool greeter);
    void setExecutable(const QString &session);
    void insertEnvironment(const QProcessEnvironment &env);

    void start();

private:
    QString m_user;
    bool m_isGreeter{false};
    QString m_sessionPath;
    QProcessEnvironment m_environment;
};
}

#endif // PLASMALOGIN_SESSIONRUNNER_H

