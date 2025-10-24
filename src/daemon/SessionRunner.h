#ifndef PLASMALOGIN_SESSIONRUNNER_H
#define PLASMALOGIN_SESSIONRUNNER_H

#include <QObject>
#include <QProcessEnvironment>
#include <QString>

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
    void setTty(int tty);

    [[nodiscard]] bool start();
    [[nodiscard]] bool stop();
    [[nodiscard]] bool isRunning() const;

private:
    QString m_user;
    bool m_isGreeter = false;
    int m_tty = 0;
    QString m_executable;
    QProcessEnvironment m_environment;
    QString m_unit;
};
}

#endif // PLASMALOGIN_SESSIONRUNNER_H
