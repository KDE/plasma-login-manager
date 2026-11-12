/*
 *  SPDX-FileCopyrightText: 2026 Alexander Berg <wehrwolfmann@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "models/sessionmodel.h"

class SessionModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testSessions();

private:
    void writeSession(const QString &dir, const QString &fileName, const QByteArray &name, const QByteArray &extra = {});

    QTemporaryDir m_highDir;
    QTemporaryDir m_lowDir;
};

void SessionModelTest::writeSession(const QString &dir, const QString &fileName, const QByteArray &name, const QByteArray &extra)
{
    QVERIFY(QDir().mkpath(dir));
    QFile file(dir + QLatin1Char('/') + fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[Desktop Entry]\nType=Application\nName=" + name + "\nExec=true\n" + extra);
}

void SessionModelTest::initTestCase()
{
    QVERIFY(m_highDir.isValid());
    QVERIFY(m_lowDir.isValid());

    // XDG_DATA_HOME takes precedence over XDG_DATA_DIRS
    qputenv("XDG_DATA_HOME", m_highDir.path().toUtf8());
    qputenv("XDG_DATA_DIRS", m_lowDir.path().toUtf8());

    const QString highWayland = m_highDir.path() + QStringLiteral("/wayland-sessions");
    const QString lowWayland = m_lowDir.path() + QStringLiteral("/wayland-sessions");
    const QString lowX = m_lowDir.path() + QStringLiteral("/xsessions");

    writeSession(lowWayland, QStringLiteral("zeta.desktop"), "Zeta");
    writeSession(lowWayland, QStringLiteral("alpha.desktop"), "Alpha");
    writeSession(lowX, QStringLiteral("alpha.desktop"), "Alpha");
    writeSession(lowWayland, QStringLiteral("plasma.desktop"), "Plasma");
    writeSession(lowX, QStringLiteral("onlyshowin.desktop"), "OnlyShowIn", "OnlyShowIn=SomethingElse;\n");

    // The higher-priority file wins, also when it hides the session
    writeSession(lowWayland, QStringLiteral("overridden.desktop"), "Overridden Low");
    writeSession(highWayland, QStringLiteral("overridden.desktop"), "Overridden High");
    writeSession(lowWayland, QStringLiteral("masked.desktop"), "Masked");
    writeSession(highWayland, QStringLiteral("masked.desktop"), "Masked", "Hidden=true\n");

    writeSession(lowWayland, QStringLiteral("hidden.desktop"), "Hidden", "Hidden=true\n");
    writeSession(lowX, QStringLiteral("nodisplay.desktop"), "NoDisplay", "NoDisplay=true\n");
}

void SessionModelTest::testSessions()
{
    SessionModel model;

    struct Row {
        QString fileName;
        Session::Type type;
        QString display;
    };

    // Plasma first, then alphabetical, then Wayland before X11
    const QList<Row> expected{
        {QStringLiteral("plasma.desktop"), Session::Type::Wayland, QStringLiteral("Plasma")},
        {QStringLiteral("alpha.desktop"), Session::Type::Wayland, QStringLiteral("Alpha (Wayland)")},
        {QStringLiteral("alpha.desktop"), Session::Type::X11, QStringLiteral("Alpha (X11)")},
        {QStringLiteral("onlyshowin.desktop"), Session::Type::X11, QStringLiteral("OnlyShowIn")},
        {QStringLiteral("overridden.desktop"), Session::Type::Wayland, QStringLiteral("Overridden High")},
        {QStringLiteral("zeta.desktop"), Session::Type::Wayland, QStringLiteral("Zeta")},
    };

    QCOMPARE(model.rowCount(), expected.size());

    for (int i = 0; i < expected.size(); ++i) {
        const QModelIndex index = model.index(i, 0);
        QCOMPARE(model.data(index, SessionModel::FileNameRole).toString(), expected[i].fileName);
        QCOMPARE(model.data(index, SessionModel::TypeRole).toInt(), int(expected[i].type));
        QCOMPARE(model.data(index, Qt::DisplayRole).toString(), expected[i].display);
    }
}

QTEST_GUILESS_MAIN(SessionModelTest)

#include "sessionmodeltest.moc"
