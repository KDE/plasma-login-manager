#include "UserDatabase.h"
#include "IoSystemdUserDatabaseAdaptor.hpp"
#include <server/varlinkserver.h>

class UserDatabaseAdaptor : public IoSystemdUserdatabaseadaptor
{
public:
    UserDatabaseAdaptor(VarLinkServer *server)
        : IoSystemdUserdatabaseadaptor(server)
    {
    }

    // Long term plan, we return a record plasmalogingreeter_$seatName
    void GetUserRecord(Message<GetUserRecordIn, GetUserRecordOut> message) override
    {
        const auto request = message.parameters();
        if (!isRequestedService(request.service)) {
            message.raiseError(u"io.systemd.UserDatabase.BadService"_s); // DAVE, I need to redo errors in QtVarlink
            return;
        }

        const bool uidMatches = !request.uid || *request.uid == GreeterUid;
        const bool nameMatches = !request.userName || *request.userName == UserName;
        if (!uidMatches || !nameMatches) {
            if (request.uid && request.userName && (uidMatches != nameMatches)) {
                message.raiseError(u"io.systemd.UserDatabase.ConflictingRecordFound"_s);
            } else {
                message.raiseError(u"io.systemd.UserDatabase.NoRecordFound"_s);
            }
            return;
        }

        if ((request.uidMin && GreeterUid < *request.uidMin) || (request.uidMax && GreeterUid > *request.uidMax) || !matchesFuzzyName(request.fuzzyNames)
            || !matchesDisposition(request.dispositionMask)) {
            message.raiseError(u"io.systemd.UserDatabase.NoRecordFound"_s);
            return;
        }

        GetUserRecordOut response;
        response.record = userRecord();
        response.incomplete = false;
        qDebug() << response.toJson();
        qDebug() << "finished";
        message.finish(response);
    }

    void GetGroupRecord(Message<GetGroupRecordIn, GetGroupRecordOut> message) override
    {
        qDebug() << "group";
        if (!isRequestedService(message.parameters().service)) {
            message.raiseError(u"io.systemd.UserDatabase.BadService"_s);
            return;
        }
        message.raiseError(u"io.systemd.UserDatabase.NoRecordFound"_s);
    }

    void GetMemberships(Message<GetMembershipsIn, GetMembershipsOut> message) override
    {
        if (!isRequestedService(message.parameters().service)) {
            message.raiseError(u"io.systemd.UserDatabase.BadService"_s);
            return;
        }
        if (message.parameters().userName == UserName) {
            message.finish({UserName, "plasmalogin"});
        }

        message.raiseError(u"io.systemd.UserDatabase.NoRecordFound"_s);
    }

private:
    bool isRequestedService(const QString &service) const
    {
        return service == m_serviceName;
    }

    static bool matchesFuzzyName(const std::optional<QList<QString>> &fuzzyNames)
    {
        return !fuzzyNames || fuzzyNames->isEmpty() || fuzzyNames->contains(UserName);
    }

    static bool matchesDisposition(const std::optional<QList<QString>> &dispositionMask)
    {
        return !dispositionMask || dispositionMask->isEmpty() || dispositionMask->contains(u"regular"_s);
    }

    QJsonObject userRecord() const
    {
        return {
            {u"userName"_s, UserName},
            {u"uid"_s, GreeterUid},
            {u"gid"_s, GreeterUid},
            {u"realName"_s, u"Demo User"_s},
            {u"homeDirectory"_s, u"/var/lib/plasmalogin"_s},
            {u"shell"_s, u"/bin/sh"_s},
            {u"disposition"_s, u"regular"_s},
            {u"service"_s, m_serviceName},
        };
    }

    QString m_serviceName = u"org.kde.plasma_login_manager"_s;
};

UserDatabase::UserDatabase(QObject *parent)
    : QObject(parent)
    , m_server(new VarLinkServer(u"unix:/var/run/systemd/userdb/org.kde.plasma_login_manager"_s))
{
    m_adaptor = std::make_unique<UserDatabaseAdaptor>(m_server.get());
}

UserDatabase::~UserDatabase() = default;

bool UserDatabase::isAvailable() const
{
    // TODO!
    return true;
}
