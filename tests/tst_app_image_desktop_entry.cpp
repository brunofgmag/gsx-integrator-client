#include <QtTest/QTest>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimeZone>
#include <QtCore/QTemporaryDir>
#include "../src/infrastructure/platform/AppImageDesktopEntry.h"

namespace
{
    constexpr auto kAwkwardAppImage = R"(/home/pilot/My Apps/gsx 100%$HOME"x`y\z.AppImage)";
    constexpr auto kAwkwardAppImageExec = R"("/home/pilot/My Apps/gsx 100%%\\$HOME\\"x\\`y\\\\z.AppImage")";
    constexpr auto kInstallerEntry = "[Desktop Entry]\nType=Application\nName=GSX Integrator\nExec=\"/x\"\n";
    constexpr auto kIconBytes = "png-bytes";

    QByteArray ReadAll(const QString& path)
    {
        QFile file(path);

        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    bool SetModified(const QString& path, const QDateTime& when)
    {
        QFile file(path);

        return file.open(QIODevice::ReadWrite) && file.setFileTime(when, QFileDevice::FileModificationTime);
    }

    bool WriteAll(const QString& path, const QByteArray& bytes)
    {
        QFile file(path);

        return QFileInfo(path).dir().mkpath(QStringLiteral(".")) && file.open(QIODevice::WriteOnly)
            && file.write(bytes) == bytes.size();
    }
}

class AppImageDesktopEntryTest final : public QObject
{
    Q_OBJECT

private slots:
    static void theExecPathIsQuotedLikeTheInstallerDoes();
    static void withoutAnEntryItWritesTheHiddenEntryAndTheIcon();
    static void theWrittenExecMatchesTheInstallersQuoting();
    static void aVisibleEntryIsLeftUntouchedAndNoIconIsWritten();
    static void aDanglingSymlinkCountsAsAnExistingEntry();
    static void anIconAlreadyThereIsNotRewritten();
    static void aHiddenEntryForAnotherPathIsRewrittenForTheCurrentAppImage();
    static void anIdenticalHiddenEntryIsNotRewritten();
    static void aDataHomeThatCannotBeWrittenFailsWithoutThrowing();
};

void AppImageDesktopEntryTest::theExecPathIsQuotedLikeTheInstallerDoes()
{
    QCOMPARE(QuoteDesktopExec(QString::fromUtf8(kAwkwardAppImage)), QString::fromUtf8(kAwkwardAppImageExec));
    QCOMPARE(QuoteDesktopExec(QStringLiteral("/a b/c.AppImage")), QStringLiteral("\"/a b/c.AppImage\""));
}

void AppImageDesktopEntryTest::withoutAnEntryItWritesTheHiddenEntryAndTheIcon()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString appImage = QStringLiteral("/home/pilot/GSX Integrator.AppImage");

    QCOMPARE(EnsureAppImageDesktopEntry(appImage, dataHome.path(), kIconBytes), DesktopEntryOutcome::Written);

    const QString iconPath = dataHome.path() + QStringLiteral("/icons/hicolor/256x256/apps/gsx-integrator-client.png");
    const QString entryPath = dataHome.path() + QStringLiteral("/applications/gsx-integrator-client.desktop");
    QCOMPARE(AppImageDesktopIconPath(dataHome.path()), iconPath);
    QCOMPARE(AppImageDesktopEntryPath(dataHome.path()), entryPath);
    QCOMPARE(ReadAll(iconPath), QByteArray(kIconBytes));

    const QString entry = QString::fromUtf8(ReadAll(entryPath));
    QVERIFY(entry.startsWith(QStringLiteral("[Desktop Entry]\n")));
    QVERIFY(entry.contains(QStringLiteral("\nType=Application\n")));
    QVERIFY(entry.contains(QStringLiteral("\nName=GSX Integrator\n")));
    QVERIFY(entry.contains(QStringLiteral("\nExec=\"") + appImage + QStringLiteral("\"\n")));
    QVERIFY(entry.contains(QStringLiteral("\nTryExec=") + appImage + u'\n'));
    QVERIFY(entry.contains(QStringLiteral("\nIcon=") + iconPath + u'\n'));
    QVERIFY(entry.contains(QStringLiteral("\nTerminal=false\n")));
    QVERIFY(entry.contains(QStringLiteral("\nStartupWMClass=gsx-integrator-client\n")));
    QVERIFY(entry.contains(QStringLiteral("\nNoDisplay=true\n")));
}

void AppImageDesktopEntryTest::theWrittenExecMatchesTheInstallersQuoting()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());

    QCOMPARE(EnsureAppImageDesktopEntry(QString::fromUtf8(kAwkwardAppImage), dataHome.path(), kIconBytes),
             DesktopEntryOutcome::Written);

    const QString entry = QString::fromUtf8(ReadAll(AppImageDesktopEntryPath(dataHome.path())));
    QVERIFY(entry.contains(QStringLiteral("\nExec=") + QString::fromUtf8(kAwkwardAppImageExec) + u'\n'));
}

void AppImageDesktopEntryTest::aVisibleEntryIsLeftUntouchedAndNoIconIsWritten()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString entryPath = AppImageDesktopEntryPath(dataHome.path());
    QVERIFY(WriteAll(entryPath, kInstallerEntry));
    const QDateTime before = QFileInfo(entryPath).lastModified();
    QTest::qWait(20);

    QCOMPARE(EnsureAppImageDesktopEntry(QStringLiteral("/home/pilot/app.AppImage"), dataHome.path(), kIconBytes),
             DesktopEntryOutcome::LeftAlone);

    QCOMPARE(ReadAll(entryPath), QByteArray(kInstallerEntry));
    QCOMPARE(QFileInfo(entryPath).lastModified(), before);
    QVERIFY(!QFileInfo::exists(AppImageDesktopIconPath(dataHome.path())));
}

void AppImageDesktopEntryTest::aDanglingSymlinkCountsAsAnExistingEntry()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString entryPath = AppImageDesktopEntryPath(dataHome.path());
    QVERIFY(QFileInfo(entryPath).dir().mkpath(QStringLiteral(".")));
    QVERIFY(QFile::link(dataHome.path() + QStringLiteral("/nowhere"), entryPath));

    QCOMPARE(EnsureAppImageDesktopEntry(QStringLiteral("/home/pilot/app.AppImage"), dataHome.path(), kIconBytes),
             DesktopEntryOutcome::LeftAlone);

    QVERIFY(QFileInfo(entryPath).isSymLink());
}

void AppImageDesktopEntryTest::anIconAlreadyThereIsNotRewritten()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString iconPath = AppImageDesktopIconPath(dataHome.path());
    QVERIFY(WriteAll(iconPath, kIconBytes));
    const QDateTime before = QFileInfo(iconPath).lastModified();
    QTest::qWait(20);

    QCOMPARE(EnsureAppImageDesktopEntry(QStringLiteral("/home/pilot/app.AppImage"), dataHome.path(), kIconBytes),
             DesktopEntryOutcome::Written);

    QCOMPARE(QFileInfo(iconPath).lastModified(), before);
}

void AppImageDesktopEntryTest::aHiddenEntryForAnotherPathIsRewrittenForTheCurrentAppImage()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString entryPath = AppImageDesktopEntryPath(dataHome.path());
    const QString iconPath = AppImageDesktopIconPath(dataHome.path());
    QVERIFY(WriteAll(entryPath, BuildAppImageDesktopEntry(QStringLiteral("/old/path/x.AppImage"), iconPath).toUtf8()));
    QVERIFY(WriteAll(iconPath, "old-icon"));
    const QString current = QStringLiteral("/new/place/x.AppImage");

    QCOMPARE(EnsureAppImageDesktopEntry(current, dataHome.path(), kIconBytes), DesktopEntryOutcome::Written);

    QCOMPARE(ReadAll(entryPath), BuildAppImageDesktopEntry(current, iconPath).toUtf8());
    QCOMPARE(ReadAll(iconPath), QByteArray(kIconBytes));
}

void AppImageDesktopEntryTest::anIdenticalHiddenEntryIsNotRewritten()
{
    const QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    const QString entryPath = AppImageDesktopEntryPath(dataHome.path());
    const QString iconPath = AppImageDesktopIconPath(dataHome.path());
    const QString current = QStringLiteral("/new/place/x.AppImage");
    const QByteArray entry = BuildAppImageDesktopEntry(current, iconPath).toUtf8();
    QVERIFY(WriteAll(entryPath, entry));
    QVERIFY(WriteAll(iconPath, kIconBytes));
    const QDateTime longAgo(QDate(2020, 1, 1), QTime(12, 0), QTimeZone::UTC);
    QVERIFY(SetModified(entryPath, longAgo));
    QVERIFY(SetModified(iconPath, longAgo));

    QCOMPARE(EnsureAppImageDesktopEntry(current, dataHome.path(), kIconBytes), DesktopEntryOutcome::UpToDate);

    QCOMPARE(ReadAll(entryPath), entry);
    QCOMPARE(QFileInfo(entryPath).lastModified(), longAgo);
    QCOMPARE(QFileInfo(iconPath).lastModified(), longAgo);
}

void AppImageDesktopEntryTest::aDataHomeThatCannotBeWrittenFailsWithoutThrowing()
{
    const QTemporaryDir parent;
    QVERIFY(parent.isValid());
    const QString dataHome = parent.path() + QStringLiteral("/share");
    QVERIFY(QDir().mkpath(dataHome));
    QVERIFY(QFile::setPermissions(dataHome, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const auto restore = qScopeGuard([&dataHome]
    {
        QFile::setPermissions(dataHome, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });
    if (QFileInfo(dataHome).isWritable())
    {
        QSKIP("the process can write into a read-only directory");
    }

    DesktopEntryOutcome outcome = DesktopEntryOutcome::Written;
    QVERIFY_THROWS_NO_EXCEPTION(outcome = EnsureAppImageDesktopEntry(
        QStringLiteral("/home/pilot/app.AppImage"), dataHome, kIconBytes));

    QCOMPARE(outcome, DesktopEntryOutcome::Failed);
    QVERIFY(!QFileInfo::exists(AppImageDesktopEntryPath(dataHome)));
}

QTEST_APPLESS_MAIN(AppImageDesktopEntryTest)

#include "tst_app_image_desktop_entry.moc"
