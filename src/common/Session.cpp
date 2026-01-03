#include "Session.h"
#include "Configuration.h"

#include <KConfig>
#include <KConfigGroup>
#include <QFile>

namespace PLASMALOGIN
{

Session Session::create(Type type, const QString &name)
{
    QString filePath;
    switch (type) {
    case Session::X11Session:
        filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("/xsessions/") + name);
        break;
    case Session::WaylandSession:
        filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("/wayland-sessions/") + name);
        break;
    default:
        filePath = QString();
        break;
    }
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        auto config = KSharedConfig::openConfig(filePath, KConfig::SimpleConfig);
        return Session(type, config);
    } else {
        return Session();
    }
}

Session::Session()
    : Session(WaylandSession, KSharedConfigPtr())
{
}

Session::Session(Type type, KSharedConfigPtr desktopFile)
    : m_type(type)
    , m_desktopFile(desktopFile)
{
}

bool Session::isValid() const
{
    if (!m_desktopFile) {
        return false;
    }

    const QString tryExec = m_desktopFile->group(QStringLiteral("Desktop Entry")).readEntry(QStringLiteral("TryExec"));
    if (!tryExec.isEmpty() && !QStandardPaths::findExecutable(tryExec).isEmpty()) {
        return false;
    }
    if (m_type == X11Session) {
        static bool xServerFound = !QStandardPaths::findExecutable(mainConfig.X11.ServerPath.get()).isEmpty();
        if (!xServerFound) {
            return false;
        }
    }
    return true;
}

Session::Type Session::type() const
{
    return m_type;
}

QString Session::name() const
{
    Q_ASSERT(m_desktopFile);
    if (!m_desktopFile) {
        return QString();
    }
    return m_desktopFile->group(QStringLiteral("Desktop Entry")).readEntry(QStringLiteral("Name"));
}

QString Session::fileName() const
{
    Q_ASSERT(m_desktopFile);
    if (!m_desktopFile) {
        return QString();
    }
    return m_desktopFile->name();
}

QString Session::desktopSession() const
{
    Q_ASSERT(m_desktopFile);
    if (!m_desktopFile) {
        return QString();
    }
    return m_desktopFile->name();
}

QString Session::xdgSessionType() const
{
    switch (m_type) {
    case WaylandSession:
        return QStringLiteral("wayland");
    case X11Session:
        return QStringLiteral("x11");
    default:
        return QString();
    }
}

QString Session::exec() const
{
    Q_ASSERT(m_desktopFile);
    if (!m_desktopFile) {
        return QString();
    }
    return m_desktopFile->group(QStringLiteral("Desktop Entry")).readEntry(QStringLiteral("Exec"));
}

QString Session::desktopNames() const
{
    Q_ASSERT(m_desktopFile);
    if (!m_desktopFile) {
        return QString();
    }
    return m_desktopFile->group(QStringLiteral("Desktop Entry")).readEntry(QStringLiteral("DesktopNames"));
}

} // namespace PLASMALOGIN
