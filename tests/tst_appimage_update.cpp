#include <unistd.h>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/update/AppImageUpdate.h"

namespace
{
    bool WriteTestFile(const QString& path, const QByteArray& content)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);

        return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(content) == content.size();
    }

    QByteArray ReadTestFile(const QString& path)
    {
        QFile file(path);

        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    struct InstallTree
    {
        QTemporaryDir root;
        QString dataHome = root.filePath(QStringLiteral("data"));
        QString installDir = dataHome + QStringLiteral("/gsx-integrator-client");
        QString appImage = installDir + QStringLiteral("/gsx-integrator-client.AppImage");
        QString record = installDir + QStringLiteral("/install.ini");
        QString maintenance = installDir + QStringLiteral("/maintenance/gsx-integrator-installer.AppImage");

        InstallTree()
        {
            WriteTestFile(appImage, "old");
            WriteTestFile(maintenance, "installer");
            QSettings settings(record, QSettings::IniFormat);
            settings.setValue(QStringLiteral("DisplayVersion"), QStringLiteral("1.0.0"));
            settings.setValue(QStringLiteral("Other"), QStringLiteral("kept"));
            settings.sync();
        }
    };
}

class AppImageUpdateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void withoutAnAppImageTheUpdateSaysWhy();
    static void aReadOnlyFolderSaysWhyAndNamesTheFolder();
    static void theStagedFileSitsNextToTheAppImage();
    static void installingReplacesTheAppImageAndMakesItExecutable();
    static void installingWithoutTheStagedFileTouchesNothing();
    static void theInstallRecordGetsTheNormalisedVersionAndKeepsEverythingElse();
    static void aStandaloneAppImageNeverTouchesTheInstallRecord();
    static void aMissingInstallDirIsNotCreated();
    static void relaunchArgumentsWaitForTheOldProcessThenExecTheNewFile();
    static void relaunchStartsTheNewFileWithoutTheAppImageVariables();
};

void AppImageUpdateTest::withoutAnAppImageTheUpdateSaysWhy()
{
    for (const QString& environment : {QString(), QStringLiteral("/nowhere/x.AppImage")})
    {
        const AppImageUpdateTarget target = ResolveAppImageUpdateTarget(environment);

        QVERIFY(target.appImagePath.isEmpty());
        QVERIFY(target.error.contains(QStringLiteral("not running from an AppImage")));
    }
}

void AppImageUpdateTest::aReadOnlyFolderSaysWhyAndNamesTheFolder()
{
    if (geteuid() == 0)
    {
        QSKIP("root can write to a read-only folder");
    }

    const QTemporaryDir root;
    const QString file = root.filePath(QStringLiteral("locked/x.AppImage"));
    QVERIFY(WriteTestFile(file, "x"));
    QVERIFY(QFile::setPermissions(QFileInfo(file).absolutePath(),
                                  QFileDevice::ReadOwner | QFileDevice::ExeOwner));

    const AppImageUpdateTarget target = ResolveAppImageUpdateTarget(file);

    QVERIFY(QFile::setPermissions(QFileInfo(file).absolutePath(),
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    QVERIFY(target.error.contains(QFileInfo(file).absolutePath()));
    QVERIFY(target.error.contains(QStringLiteral("cannot write")));
}

void AppImageUpdateTest::theStagedFileSitsNextToTheAppImage()
{
    const QTemporaryDir root;
    const QString file = root.filePath(QStringLiteral("apps/x.AppImage"));
    QVERIFY(WriteTestFile(file, "x"));

    const AppImageUpdateTarget target = ResolveAppImageUpdateTarget(file);

    QVERIFY(target.error.isEmpty());
    QCOMPARE(target.appImagePath, file);
    QCOMPARE(target.stagedPath, file + QStringLiteral(".new"));
    QCOMPARE(StagedAppImagePathFor(file), target.stagedPath);
    QVERIFY(StagedAppImagePathFor(QString()).isEmpty());
}

void AppImageUpdateTest::installingReplacesTheAppImageAndMakesItExecutable()
{
    const QTemporaryDir root;
    const QString file = root.filePath(QStringLiteral("apps/x.AppImage"));
    QVERIFY(WriteTestFile(file, "old"));
    QVERIFY(WriteTestFile(file + QStringLiteral(".new"), "new"));

    QVERIFY(InstallStagedAppImage(file + QStringLiteral(".new"), file));

    QCOMPARE(ReadTestFile(file), QByteArray("new"));
    QVERIFY(QFileInfo(file).isExecutable());
    QVERIFY(!QFile::exists(file + QStringLiteral(".new")));
}

void AppImageUpdateTest::installingWithoutTheStagedFileTouchesNothing()
{
    const QTemporaryDir root;
    const QString file = root.filePath(QStringLiteral("apps/x.AppImage"));
    QVERIFY(WriteTestFile(file, "old"));

    QVERIFY(!InstallStagedAppImage(file + QStringLiteral(".new"), file));

    QCOMPARE(ReadTestFile(file), QByteArray("old"));
}

void AppImageUpdateTest::theInstallRecordGetsTheNormalisedVersionAndKeepsEverythingElse()
{
    const InstallTree tree;

    QVERIFY(RecordInstalledVersion(tree.appImage, tree.dataHome, QStringLiteral("v1.41.0-rc.1")));

    const QSettings settings(tree.record, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("DisplayVersion")).toString(), QStringLiteral("1.41.0"));
    QCOMPARE(settings.value(QStringLiteral("Other")).toString(), QStringLiteral("kept"));
    QCOMPARE(ReadTestFile(tree.maintenance), QByteArray("installer"));
    QVERIFY(ReadTestFile(tree.record).contains("DisplayVersion=1.41.0"));
}

void AppImageUpdateTest::aStandaloneAppImageNeverTouchesTheInstallRecord()
{
    const InstallTree tree;
    const QString standalone = tree.root.filePath(QStringLiteral("Downloads/gsx-integrator-client.AppImage"));
    QVERIFY(WriteTestFile(standalone, "x"));
    const QString renamed = tree.installDir + QStringLiteral("/other.AppImage");
    QVERIFY(WriteTestFile(renamed, "x"));

    QVERIFY(!RecordInstalledVersion(standalone, tree.dataHome, QStringLiteral("2.0.0")));
    QVERIFY(!RecordInstalledVersion(renamed, tree.dataHome, QStringLiteral("2.0.0")));

    QCOMPARE(QSettings(tree.record, QSettings::IniFormat).value(QStringLiteral("DisplayVersion")).toString(),
             QStringLiteral("1.0.0"));
}

void AppImageUpdateTest::aMissingInstallDirIsNotCreated()
{
    const QTemporaryDir root;

    QVERIFY(!RecordInstalledVersion(root.filePath(QStringLiteral("data/gsx-integrator-client/gsx-integrator-client.AppImage")),
                                    root.filePath(QStringLiteral("data")), QStringLiteral("2.0.0")));
    QVERIFY(!QFileInfo::exists(root.filePath(QStringLiteral("data"))));
}

void AppImageUpdateTest::relaunchArgumentsWaitForTheOldProcessThenExecTheNewFile()
{
    const QStringList arguments = AppImageRelaunchArguments(4242, QStringLiteral("/x/gsx.AppImage"));

    QCOMPARE(arguments.size(), 4);
    QCOMPARE(arguments[0], QStringLiteral("-c"));
    QVERIFY(arguments[1].contains(QStringLiteral("kill -0 \"$1\"")));
    QVERIFY(arguments[1].contains(QStringLiteral("exec \"$0\"")));
    QCOMPARE(arguments[2], QStringLiteral("/x/gsx.AppImage"));
    QCOMPARE(arguments[3], QStringLiteral("4242"));
}

void AppImageUpdateTest::relaunchStartsTheNewFileWithoutTheAppImageVariables()
{
    const QTemporaryDir root;
    const QString marker = root.filePath(QStringLiteral("marker"));
    const QString app = root.filePath(QStringLiteral("new.AppImage"));
    QVERIFY(WriteTestFile(app, QStringLiteral("#!/bin/sh\necho \"started appimage=[$APPIMAGE] appdir=[$APPDIR]\" > \"%1\"\n")
                                   .arg(marker).toUtf8()));
    QVERIFY(QFile::setPermissions(app, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));

    QProcess finished;
    finished.start(QStringLiteral("/bin/true"));
    QVERIFY(finished.waitForFinished());
    qputenv("APPIMAGE", "/old/x.AppImage");
    qputenv("APPDIR", "/tmp/.mount_old");

    QVERIFY(RelaunchAppImage(finished.processId(), app));

    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(marker) && !ReadTestFile(marker).isEmpty(), 5000);
    QCOMPARE(ReadTestFile(marker), QByteArray("started appimage=[] appdir=[]\n"));
    qunsetenv("APPIMAGE");
    qunsetenv("APPDIR");
}

QTEST_GUILESS_MAIN(AppImageUpdateTest)

#include "tst_appimage_update.moc"
