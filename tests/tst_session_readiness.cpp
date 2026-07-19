#include <QtTest/QTest>

#include <string>
#include "../src/application/sim/SessionReadiness.h"
#include "../src/application/sim/SimVersion.h"

Q_DECLARE_METATYPE(SimVersion)

class SessionReadinessTest final : public QObject
{
    Q_OBJECT

private slots:
    static void evaluatesTruthTable_data();
    static void evaluatesTruthTable();
    static void cameraBoundaryAtEleven();
    static void fs2020IgnoresAircraftAndAvatarVars();
    static void detectsSimVersionFromAppName_data();
    static void detectsSimVersionFromAppName();
    static void labelsSimVersions();
};

void SessionReadinessTest::evaluatesTruthTable_data()
{
    QTest::addColumn<SimVersion>("version");
    QTest::addColumn<double>("cameraState");
    QTest::addColumn<double>("isAircraft");
    QTest::addColumn<double>("isAvatar");
    QTest::addColumn<bool>("expected");

    // FS2024
    //                                                    version   camera  aircraft  avatar  expected
    QTest::newRow("2024 cockpit ready") << SimVersion::Msfs2024 << 2.0 << 1.0 << 0.0 << true;
    QTest::newRow("2024 cockpit lower bound") << SimVersion::Msfs2024 << 10.0 << 1.0 << 0.0 << true;
    QTest::newRow("2024 global menu") << SimVersion::Msfs2024 << 11.0 << 1.0 << 0.0 << false;
    QTest::newRow("2024 external camera") << SimVersion::Msfs2024 << 25.0 << 1.0 << 0.0 << false;
    QTest::newRow("2024 no aircraft") << SimVersion::Msfs2024 << 4.0 << 0.0 << 0.0 << false;
    QTest::newRow("2024 aircraft and avatar") << SimVersion::Msfs2024 << 4.0 << 1.0 << 1.0 << false;
    QTest::newRow("2024 walkaround avatar") << SimVersion::Msfs2024 << 4.0 << 0.0 << 1.0 << false;
    QTest::newRow("2024 nominal") << SimVersion::Msfs2024 << 4.0 << 1.0 << 0.0 << true;

    // FS2020
    QTest::newRow("2020 cockpit ready") << SimVersion::Msfs2020 << 2.0 << 0.0 << 0.0 << true;
    QTest::newRow("2020 cockpit lower bound") << SimVersion::Msfs2020 << 10.0 << 0.0 << 0.0 << true;
    QTest::newRow("2020 global menu") << SimVersion::Msfs2020 << 11.0 << 0.0 << 0.0 << false;
    QTest::newRow("2020 external camera") << SimVersion::Msfs2020 << 25.0 << 0.0 << 0.0 << false;

    // Unknown Sim
    QTest::newRow("unknown camera only ready") << SimVersion::Unknown << 4.0 << 0.0 << 0.0 << true;
    QTest::newRow("unknown menu") << SimVersion::Unknown << 11.0 << 0.0 << 0.0 << false;

    QTest::newRow("2024 camera zero") << SimVersion::Msfs2024 << 0.0 << 1.0 << 0.0 << true;
    QTest::newRow("2020 camera negative") << SimVersion::Msfs2020 << -5.0 << 0.0 << 0.0 << true;
}

void SessionReadinessTest::evaluatesTruthTable()
{
    QFETCH(SimVersion, version);
    QFETCH(double, cameraState);
    QFETCH(double, isAircraft);
    QFETCH(double, isAvatar);
    QFETCH(bool, expected);

    QCOMPARE(SessionReadiness::Evaluate(version, cameraState, isAircraft, isAvatar), expected);
}

void SessionReadinessTest::cameraBoundaryAtEleven()
{
    QVERIFY(SessionReadiness::Evaluate(SimVersion::Msfs2024, 10.0, 1.0, 0.0));
    QVERIFY(!SessionReadiness::Evaluate(SimVersion::Msfs2024, 11.0, 1.0, 0.0));
    QVERIFY(SessionReadiness::Evaluate(SimVersion::Msfs2020, 10.0, 0.0, 0.0));
    QVERIFY(!SessionReadiness::Evaluate(SimVersion::Msfs2020, 11.0, 0.0, 0.0));
}

void SessionReadinessTest::fs2020IgnoresAircraftAndAvatarVars()
{
    QVERIFY(SessionReadiness::Evaluate(SimVersion::Msfs2020, 4.0, 0.0, 0.0));
    QVERIFY(SessionReadiness::Evaluate(SimVersion::Msfs2020, 4.0, 0.0, 1.0));
    QVERIFY(SessionReadiness::Evaluate(SimVersion::Unknown, 4.0, 1.0, 1.0));
}

void SessionReadinessTest::detectsSimVersionFromAppName_data()
{
    QTest::addColumn<QString>("appName");
    QTest::addColumn<SimVersion>("expected");

    QTest::newRow("KittyHawk -> 2020") << QStringLiteral("KittyHawk") << SimVersion::Msfs2020;
    QTest::newRow("SunRise -> 2024") << QStringLiteral("SunRise") << SimVersion::Msfs2024;
    QTest::newRow("lowercase kittyhawk") << QStringLiteral("kittyhawk") << SimVersion::Msfs2020;
    QTest::newRow("uppercase SUNRISE") << QStringLiteral("SUNRISE") << SimVersion::Msfs2024;
    QTest::newRow("empty -> unknown") << QString() << SimVersion::Unknown;
    QTest::newRow("other -> unknown") << QStringLiteral("Prepar3D") << SimVersion::Unknown;
}

void SessionReadinessTest::detectsSimVersionFromAppName()
{
    QFETCH(QString, appName);
    QFETCH(SimVersion, expected);

    const std::string name = appName.toStdString();

    QCOMPARE(SimVersionDetect::FromAppName(name), expected);
}

void SessionReadinessTest::labelsSimVersions()
{
    QCOMPARE(SimVersionLabel(SimVersion::Msfs2020), "MSFS 2020");
    QCOMPARE(SimVersionLabel(SimVersion::Msfs2024), "MSFS 2024");
    QCOMPARE(SimVersionLabel(SimVersion::Unknown), "Unknown");
}

QTEST_APPLESS_MAIN(SessionReadinessTest)

#include "tst_session_readiness.moc"
