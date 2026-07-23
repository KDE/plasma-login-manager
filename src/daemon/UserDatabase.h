#include <QDebug>
#include <QFile>
#include <QJsonObject>

#include <memory>

using namespace Qt::StringLiterals;

constexpr qint64 GreeterUid = 20000; // FIXME, I found some docs saying which UID range to use, but now can't find it
const QString UserName = u"plasmalogingreeter2"_s;

class UserDatabaseAdaptor;
class VarLinkServer;

class UserDatabase : public QObject
{
    Q_OBJECT
public:
    UserDatabase(QObject *parent);
    bool isAvailable() const;
    ~UserDatabase();

private:
    std::unique_ptr<VarLinkServer> m_server;
    std::unique_ptr<UserDatabaseAdaptor> m_adaptor;
};
