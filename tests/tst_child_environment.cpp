#include <QtTest/QTest>

#include "../src/infrastructure/platform/ChildEnvironment.h"

class ChildEnvironmentTest final : public QObject
{
    Q_OBJECT

private slots:
    static void dropsTheVariablesTheAppImageRuntimeSets();
    static void stripsPathEntriesInsideTheMount();
    static void removesAPathVariableLeftEmpty();
    static void leavesAnEnvironmentWithoutAnAppImageAlone();
    static void doesNotTouchSimilarlyNamedDirectories();
};

void ChildEnvironmentTest::dropsTheVariablesTheAppImageRuntimeSets()
{
    QProcessEnvironment env;
    env.insert(QStringLiteral("APPIMAGE"), QStringLiteral("/home/u/i.AppImage"));
    env.insert(QStringLiteral("APPDIR"), QStringLiteral("/tmp/.mount_x"));
    env.insert(QStringLiteral("ARGV0"), QStringLiteral("i"));
    env.insert(QStringLiteral("OWD"), QStringLiteral("/home/u"));
    env.insert(QStringLiteral("HOME"), QStringLiteral("/home/u"));

    const QProcessEnvironment result = ChildProcessEnvironment(env);

    QVERIFY(!result.contains(QStringLiteral("APPIMAGE")));
    QVERIFY(!result.contains(QStringLiteral("APPDIR")));
    QVERIFY(!result.contains(QStringLiteral("ARGV0")));
    QVERIFY(!result.contains(QStringLiteral("OWD")));
    QCOMPARE(result.value(QStringLiteral("HOME")), QStringLiteral("/home/u"));
}

void ChildEnvironmentTest::stripsPathEntriesInsideTheMount()
{
    QProcessEnvironment env;
    env.insert(QStringLiteral("APPDIR"), QStringLiteral("/tmp/.mount_x"));
    env.insert(QStringLiteral("LD_LIBRARY_PATH"), QStringLiteral("/tmp/.mount_x/usr/lib:/opt/other/lib"));
    env.insert(QStringLiteral("PATH"), QStringLiteral("/tmp/.mount_x/usr/bin:/usr/bin:/tmp/.mount_x"));
    env.insert(QStringLiteral("QT_PLUGIN_PATH"), QStringLiteral("/tmp/.mount_x/usr/plugins"));

    const QProcessEnvironment result = ChildProcessEnvironment(env);

    QCOMPARE(result.value(QStringLiteral("LD_LIBRARY_PATH")), QStringLiteral("/opt/other/lib"));
    QCOMPARE(result.value(QStringLiteral("PATH")), QStringLiteral("/usr/bin"));
    QVERIFY(!result.contains(QStringLiteral("QT_PLUGIN_PATH")));
}

void ChildEnvironmentTest::removesAPathVariableLeftEmpty()
{
    QProcessEnvironment env;
    env.insert(QStringLiteral("APPDIR"), QStringLiteral("/tmp/.mount_x"));
    env.insert(QStringLiteral("LD_LIBRARY_PATH"), QStringLiteral("/tmp/.mount_x/usr/lib"));

    QVERIFY(!ChildProcessEnvironment(env).contains(QStringLiteral("LD_LIBRARY_PATH")));
}

void ChildEnvironmentTest::leavesAnEnvironmentWithoutAnAppImageAlone()
{
    QProcessEnvironment env;
    env.insert(QStringLiteral("LD_LIBRARY_PATH"), QStringLiteral("/opt/lib"));
    env.insert(QStringLiteral("PATH"), QStringLiteral("/usr/bin"));

    const QProcessEnvironment result = ChildProcessEnvironment(env);

    QCOMPARE(result.value(QStringLiteral("LD_LIBRARY_PATH")), QStringLiteral("/opt/lib"));
    QCOMPARE(result.value(QStringLiteral("PATH")), QStringLiteral("/usr/bin"));
}

void ChildEnvironmentTest::doesNotTouchSimilarlyNamedDirectories()
{
    QProcessEnvironment env;
    env.insert(QStringLiteral("APPDIR"), QStringLiteral("/tmp/.mount_x"));
    env.insert(QStringLiteral("PATH"), QStringLiteral("/tmp/.mount_xyz/bin:/usr/bin"));

    QCOMPARE(ChildProcessEnvironment(env).value(QStringLiteral("PATH")), QStringLiteral("/tmp/.mount_xyz/bin:/usr/bin"));
}

QTEST_APPLESS_MAIN(ChildEnvironmentTest)

#include "tst_child_environment.moc"
