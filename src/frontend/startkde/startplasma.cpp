/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
    SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// #include <config-startplasma.h>

// #include <canberra.h>

#include <ranges>

#include "debug.h"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QStandardPaths>

#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <KConfig>
#include <KConfigGroup>
// #include <KDarkLightSchedule>
// #include <KNotifyConfig>
#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KSharedConfig>

#include <signal.h>
#include <unistd.h>

// #include <autostartscriptdesktopfile.h>

#include <KUpdateLaunchEnvironmentJob>

#include "startplasma.h"

// #include "../config-workspace.h"
#include "debug.h"
#include "klookandfeelmanager.h"
#include "lookandfeelsettings.h"

using namespace Qt::StringLiterals;

QTextStream out(stderr);

void sigtermHandler(int signalNumber)
{
    Q_UNUSED(signalNumber)
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->exit(-1);
    }
}

QStringList allServices(const QLatin1String &prefix)
{
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QStringList names;

    std::copy_if(services.cbegin(), services.cend(), std::back_inserter(names), [&prefix](const QString &serviceName) {
        return serviceName.startsWith(prefix);
    });

    return names;
}

void gentleTermination(QProcess *p)
{
    if (p->state() != QProcess::Running) {
        return;
    }

    p->terminate();

    // Wait longer for a session than a greeter
    if (!p->waitForFinished(5000)) {
        p->kill();
        if (!p->waitForFinished(5000)) {
            qCWarning(PLASMA_STARTUP) << "Could not fully finish the process" << p->program();
        }
    }
}

int runSync(const QString &program, const QStringList &args, const QStringList &env)
{
    QProcess p;
    if (!env.isEmpty()) {
        p.setEnvironment(QProcess::systemEnvironment() << env);
    }
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(program, args);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &p, [&p] {
        gentleTermination(&p);
    });
    qCDebug(PLASMA_STARTUP) << "started..." << program << args;
    p.waitForFinished(-1);
    if (p.exitCode()) {
        qCWarning(PLASMA_STARTUP) << program << args << "exited with code" << p.exitCode();
    }
    return p.exitCode();
}

template<typename T>
concept ViewType = std::same_as<QByteArrayView, T> || std::same_as<QStringView, T>;
inline bool isShellVariable(ViewType auto name)
{
    return name == "_"_L1 || name == "SHELL"_L1 || name.startsWith("SHLVL"_L1);
}

inline bool isConfinementVariable(QStringView name)
{
    return name == "SNAP"_L1 || name.startsWith("SNAP_"_L1);
}

inline bool isSessionVariable(QStringView name)
{
    // Check is variable is specific to session.
    return name == "DISPLAY"_L1 || name == "XAUTHORITY"_L1 || //
        name == "WAYLAND_DISPLAY"_L1 || name == "WAYLAND_SOCKET"_L1 || //
        name.startsWith("XDG_"_L1);
}

void setEnvironmentVariable(const char *name, QByteArrayView value)
{
    const QByteArray currentValue = qgetenv(name);
    if (currentValue.isNull() || currentValue != value) {
        qputenv(name, value);
    }
}

void createConfigDirectory()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!QDir().mkpath(configDir)) {
        out << "Could not create config directory XDG_CONFIG_HOME: " << configDir << '\n';
    }
}

void runStartupConfig()
{
    // export LC_* variables set by kcmshell5 formats into environment
    // so it can be picked up by QLocale and friends.
    KConfig config(QStringLiteral("plasma-localerc"));
    KConfigGroup formatsConfig = KConfigGroup(&config, QStringLiteral("Formats"));

    // Note: not all of these (e.g. LC_CTYPE) can currently be changed through system settings (but they can be changed by modifying
    // plasma-localrc manually).
    const auto lcValues = {"LANG",
                           "LC_ADDRESS",
                           "LC_COLLATE",
                           "LC_CTYPE",
                           "LC_IDENTIFICATION",
                           "LC_MONETARY",
                           "LC_MESSAGES",
                           "LC_MEASUREMENT",
                           "LC_NAME",
                           "LC_NUMERIC",
                           "LC_PAPER",
                           "LC_TELEPHONE",
                           "LC_TIME",
                           "LC_ALL"};
    for (auto lc : lcValues) {
        const QString value = formatsConfig.readEntry(lc, QString());
        if (!value.isEmpty()) {
            qputenv(lc, value.toUtf8());
        }
    }

    KConfigGroup languageConfig = KConfigGroup(&config, QStringLiteral("Translations"));
    const QString value = languageConfig.readEntry("LANGUAGE", QString());
    if (!value.isEmpty()) {
        qputenv("LANGUAGE", value.toUtf8());
    }

    if (!formatsConfig.hasKey("LANG") && !qEnvironmentVariableIsEmpty("LANG")) {
        formatsConfig.writeEntry("LANG", qgetenv("LANG"));
        formatsConfig.sync();
    }
}

std::optional<QProcessEnvironment> getSystemdEnvironment()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("Get"));
    msg << QStringLiteral("org.freedesktop.systemd1.Manager") << QStringLiteral("Environment");
    auto reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        return std::nullopt;
    }

    // Make sure the returned type is correct.
    auto arguments = reply.arguments();
    if (arguments.isEmpty() || arguments[0].userType() != qMetaTypeId<QDBusVariant>()) {
        return std::nullopt;
    }
    auto variant = qdbus_cast<QVariant>(arguments[0]);
    if (variant.typeId() != QMetaType::QStringList) {
        return std::nullopt;
    }

    const auto assignmentList = variant.toStringList();
    QProcessEnvironment ret;
    for (auto &env : assignmentList) {
        const int idx = env.indexOf(QLatin1Char('='));
        if (Q_LIKELY(idx > 0)) {
            ret.insert(env.left(idx), env.mid(idx + 1));
        }
    }

    return ret;
}

// Import systemd user environment.
//
// Systemd read ~/.config/environment.d which applies to all systemd user unit.
// But it won't work if plasma is not started by systemd.
void importSystemdEnvrionment()
{
    const auto environment = getSystemdEnvironment();
    if (!environment) {
        return;
    }

    const auto keys = environment.value().keys();
    for (const QString &nameStr : keys) {
        if (!isShellVariable(QStringView(nameStr)) && !isSessionVariable(nameStr)) {
            setEnvironmentVariable(nameStr.toLocal8Bit().constData(), environment.value().value(nameStr).toLocal8Bit());
        }
    }
}

/*
static std::optional<std::pair<QString, KLookAndFeelManager::Contents>> dayNightLookAndFeel(const LookAndFeelSettings &settings)
{
    const KConfig lookandfeelautoswitcherstaterc(QStringLiteral("lookandfeelautoswitcherstaterc"), KConfig::SimpleConfig, QStandardPaths::GenericStateLocation);
    const KConfigGroup darkNightCycleGroup(&lookandfeelautoswitcherstaterc, QStringLiteral("DarkLightCycle"));
    if (!darkNightCycleGroup.isValid()) {
        return std::nullopt;
    }

    const std::optional<KDarkLightSchedule> darkLightSchedule =
        KDarkLightSchedule::fromState(darkNightCycleGroup.readEntry(QStringLiteral("SerializedSchedule")));
    if (!darkLightSchedule) {
        return std::nullopt;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const auto previousTransition = darkLightSchedule->previousTransition(now);

    bool wantsDarkTheme = false;
    switch (previousTransition->test(now)) {
    case KDarkLightTransition::Upcoming:
    case KDarkLightTransition::InProgress:
        wantsDarkTheme = previousTransition->type() == KDarkLightTransition::Morning;
        break;
    case KDarkLightTransition::Passed:
        wantsDarkTheme = previousTransition->type() != KDarkLightTransition::Morning;
        break;
    }

    const QString lookAndFeelName = wantsDarkTheme ? settings.defaultDarkLookAndFeel() : settings.defaultLightLookAndFeel();
    return std::make_pair(lookAndFeelName, KLookAndFeelManager::AppearanceSettings);
}
*/

static std::pair<QString, KLookAndFeelManager::Contents> determineLookAndFeel()
{
    const LookAndFeelSettings settings;

    /*
    if (settings.automaticLookAndFeel()) {
        if (const auto lookAndFeel = dayNightLookAndFeel(settings)) {
            return *lookAndFeel;
        }
    }
    */

    return std::make_pair(settings.lookAndFeelPackage(), KLookAndFeelManager::AllSettings);
}

void setupPlasmaEnvironment()
{
    // Manually disable auto scaling because we are scaling above
    // otherwise apps that manually opt in for high DPI get auto scaled by the developer AND manually scaled by us
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    qputenv("QT_QPA_PLATFORMTHEME", "kde"); // KDE doesn't load by default as we don't register as a full session
    qputenv("KDE_APPLICATIONS_AS_SCOPE", "1");

    // Add kdedefaults dir to allow config defaults overriding from a writable location
    QByteArray currentConfigDirs = qgetenv("XDG_CONFIG_DIRS");
    if (currentConfigDirs.isEmpty()) {
        currentConfigDirs = "/etc/xdg";
    }
    const QString extraConfigDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults");
    QDir().mkpath(extraConfigDir);
    qputenv("XDG_CONFIG_DIRS", QByteArray(QFile::encodeName(extraConfigDir) + ':' + currentConfigDirs));

    const auto &[lookAndFeelName, lookAndFeelContents] = determineLookAndFeel();
    QFile activeLnf(extraConfigDir + QLatin1String("/package"));
    activeLnf.open(QIODevice::ReadOnly);
    if (activeLnf.readLine() != lookAndFeelName.toUtf8()) {
        KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), lookAndFeelName);
        KLookAndFeelManager lnfManager;
        lnfManager.setMode(KLookAndFeelManager::Mode::Defaults);
        lnfManager.save(package, lookAndFeelContents);
    }
    // check if colors changed, if so apply them and discard plasma cache
    {
        KLookAndFeelManager lnfManager;
        lnfManager.setMode(KLookAndFeelManager::Mode::Apply);
        KConfig globals(QStringLiteral("kdeglobals")); // Reload the config
        KConfigGroup generalGroup(&globals, QStringLiteral("General"));
        const QString colorScheme = generalGroup.readEntry("ColorScheme", QStringLiteral("BreezeLight"));
        QString path = lnfManager.colorSchemeFile(colorScheme);

        if (!path.isEmpty()) {
            QFile f(path);
            QCryptographicHash hash(QCryptographicHash::Sha1);
            if (f.open(QFile::ReadOnly) && hash.addData(&f)) {
                const QString fileHash = QString::fromUtf8(hash.result().toHex());
                if (fileHash != generalGroup.readEntry("ColorSchemeHash", QString())) {
                    lnfManager.setColors(colorScheme, path);
                    generalGroup.writeEntry("ColorSchemeHash", fileHash);
                    generalGroup.sync();
                }
            }
        }
    }
}

void cleanupPlasmaEnvironment(const std::optional<QProcessEnvironment> &oldSystemdEnvironment)
{
    if (!oldSystemdEnvironment) {
        return;
    }

    auto currentEnv = getSystemdEnvironment();
    if (!currentEnv) {
        return;
    }

    // According to systemd documentation:
    // If a variable is listed in both, the variable is set after this method returns, i.e. the set list overrides the unset list.
    // So this will effectively restore the state to the values in oldSystemdEnvironment.
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("UnsetAndSetEnvironment"));
    message.setArguments({currentEnv.value().keys(), oldSystemdEnvironment.value().toStringList()});

    // The session program gonna quit soon, ensure the message is flushed.
    auto reply = QDBusConnection::sessionBus().asyncCall(message);
    reply.waitForFinished();
}

// Drop session-specific variables from the systemd environment.
// Those can be leftovers from previous sessions, which can interfere with the session
// we want to start now, e.g. $DISPLAY might break kwin_wayland.
static void dropSessionVarsFromSystemdEnvironment()
{
    const auto environment = getSystemdEnvironment();
    if (!environment) {
        return;
    }

    QStringList varsToDrop;
    const auto keys = environment.value().keys();
    for (const QString &nameStr : keys) {
        // If it's set in this process, it'll be overwritten by the following UpdateLaunchEnvJob
        if (!qEnvironmentVariableIsSet(nameStr.toLocal8Bit().constData()) && isSessionVariable(nameStr)) {
            varsToDrop.append(nameStr);
        }
    }

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                              QStringLiteral("UnsetEnvironment"));
    msg << varsToDrop;
    auto reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        // qCWarning(PLASMA_STARTUP) << "Failed to unset systemd environment variables:" << reply.errorName() << reply.errorMessage();
    }
}

// kwin_wayland can possibly also start dbus-activated services which need env variables.
// In that case, the update in startplasma might be too late.
bool syncDBusEnvironment()
{
    dropSessionVarsFromSystemdEnvironment();

    // Shell and confinement variables are filtered out of things we explicitly load, but they
    // still might have been inherited from the parent process
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const auto keys = environment.keys();
    for (const QString &name : keys) {
        if (isShellVariable(QStringView(name)) || isConfinementVariable(QStringView(name))) {
            environment.remove(name);
        }
    }

    // At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
    auto job = new KUpdateLaunchEnvironmentJob(environment);
    QEventLoop e;
    QObject::connect(job, &KUpdateLaunchEnvironmentJob::finished, &e, &QEventLoop::quit);
    e.exec();
    return true;
}

// If something went on an endless restart crash loop it will get blacklisted, as this is a clean login we will want to reset those counters
// This is independent of whether we use the Plasma systemd boot
void resetSystemdFailedUnits()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                          QStringLiteral("/org/freedesktop/systemd1"),
                                                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                          QStringLiteral("ResetFailed"));
    QDBusConnection::sessionBus().call(message);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    createConfigDirectory();
    signal(SIGTERM, sigtermHandler);

    // Let clients try to reconnect to kwin after a restart
    qputenv("QT_WAYLAND_RECONNECT", "1");

    // Query whether org.freedesktop.locale1 is available. If it is, try to
    // set XKB_DEFAULT_{MODEL,LAYOUT,VARIANT,OPTIONS} accordingly.
    {
        const QString locale1Service = QStringLiteral("org.freedesktop.locale1");
        const QString locale1Path = QStringLiteral("/org/freedesktop/locale1");
        QDBusMessage message =
            QDBusMessage::createMethodCall(locale1Service, locale1Path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));
        message << locale1Service;
        QDBusMessage resultMessage = QDBusConnection::systemBus().call(message);
        if (resultMessage.type() == QDBusMessage::ReplyMessage) {
            QVariantMap result;
            QDBusArgument dbusArgument = resultMessage.arguments().at(0).value<QDBusArgument>();
            while (!dbusArgument.atEnd()) {
                dbusArgument >> result;
            }

            auto queryAndSet = [&result](const char *var, const QString &value) {
                const auto r = result.value(value).toString();
                if (!r.isEmpty()) {
                    qputenv(var, r.toUtf8());
                }
            };

            queryAndSet("XKB_DEFAULT_MODEL", QStringLiteral("X11Model"));
            queryAndSet("XKB_DEFAULT_LAYOUT", QStringLiteral("X11Layout"));
            queryAndSet("XKB_DEFAULT_VARIANT", QStringLiteral("X11Variant"));
            queryAndSet("XKB_DEFAULT_OPTIONS", QStringLiteral("X11Options"));
        } else {
            qWarning() << "not a reply org.freedesktop.locale1" << resultMessage;
        }
    }

    setupPlasmaEnvironment();
    runStartupConfig();

    auto oldSystemdEnvironment = getSystemdEnvironment();
    if (!syncDBusEnvironment()) {
        out << "Could not sync environment to dbus.\n";
        return 1;
    };

    {
        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                  QStringLiteral("/org/freedesktop/systemd1"),
                                                  QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                  QStringLiteral("StartUnit"));
        msg << QStringLiteral("plasma-login-wayland.target") << QStringLiteral("fail");
        QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
        if (!reply.isValid()) {
            qWarning() << "Could not start systemd managed Plasma session:" << reply.error().name() << reply.error().message();
        }
    }

    // stopped by the sigterm handler
    app.exec();

    qDebug() << "stopping";

    {
        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                  QStringLiteral("/org/freedesktop/systemd1"),
                                                  QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                  QStringLiteral("StopUnit"));
        msg << QStringLiteral("plasma-login-wayland.target") << QStringLiteral("fail");
        QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
        if (!reply.isValid()) {
            qWarning() << "Could not stop systemd managed Plasma session:" << reply.error().name() << reply.error().message();
        }
    }

    qDebug() << "final cleanup";

    // systemd returns when the call is made, but not all jobs are torn down
    // this waits until kwin is definitely gone too, which helps logind
    {
        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                  QStringLiteral("/org/freedesktop/systemd1"),
                                                  QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                  QStringLiteral("StopUnit"));
        msg << QStringLiteral("plasma-login-kwin_wayland.service") << QStringLiteral("fail");
        QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
        if (!reply.isValid()) {
            qWarning() << "Could not close up systemd managed Plasma session:" << reply.error().name() << reply.error().message();
        }
    }

    return 0;
}
