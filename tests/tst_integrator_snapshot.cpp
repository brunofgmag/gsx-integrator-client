#include <QtTest/QTest>

#include "../src/application/model/IntegratorSnapshot.h"

class IntegratorSnapshotTest final : public QObject
{
    Q_OBJECT

private slots:
    static void defaultSnapshotsAreEquivalent();
    static void boolFieldDifferenceBreaksEquivalence();
    static void aircraftNameDifferenceBreaksEquivalence();
    static void phaseDifferenceBreaksEquivalence();
    static void flightPlanStatusDifferenceBreaksEquivalence();
    static void plannedPaxDifferenceBreaksEquivalence();
    static void floatDifferenceBelowEpsilonStaysEquivalent();
    static void floatDifferenceAboveEpsilonBreaksEquivalence();
};

void IntegratorSnapshotTest::defaultSnapshotsAreEquivalent()
{
    QVERIFY(AreEquivalent(IntegratorSnapshot{}, IntegratorSnapshot{}));
}

void IntegratorSnapshotTest::boolFieldDifferenceBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.connected = true;

    QVERIFY(!AreEquivalent(a, b));

    b.connected = false;
    b.sessionActive = true;

    QVERIFY(!AreEquivalent(a, b));

    b.sessionActive = false;
    b.automationEnabled = true;

    QVERIFY(!AreEquivalent(a, b));

    b.automationEnabled = false;
    b.gsxAvailable = true;

    QVERIFY(!AreEquivalent(a, b));

    b.gsxAvailable = false;
    b.aircraftSupported = true;

    QVERIFY(!AreEquivalent(a, b));

    b.aircraftSupported = false;
    b.canToggleAutomation = true;

    QVERIFY(!AreEquivalent(a, b));

    b.canToggleAutomation = false;
    b.canStartLoading = true;

    QVERIFY(!AreEquivalent(a, b));

    b.canStartLoading = false;
    b.canReloadSimbrief = true;

    QVERIFY(!AreEquivalent(a, b));
}

void IntegratorSnapshotTest::aircraftNameDifferenceBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.aircraftName = "MD-11";

    QVERIFY(!AreEquivalent(a, b));
}

void IntegratorSnapshotTest::phaseDifferenceBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.phase = TurnaroundPhase::Refueling;

    QVERIFY(!AreEquivalent(a, b));
}

void IntegratorSnapshotTest::flightPlanStatusDifferenceBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.flightPlanStatus = FlightPlanStatus::Ready;

    QVERIFY(!AreEquivalent(a, b));
}

void IntegratorSnapshotTest::plannedPaxDifferenceBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.plannedPax = 1;

    QVERIFY(!AreEquivalent(a, b));
}

void IntegratorSnapshotTest::floatDifferenceBelowEpsilonStaysEquivalent()
{
    IntegratorSnapshot a;
    IntegratorSnapshot b;

    a.fuelProgress = 10.0;
    b.fuelProgress = 10.0005;
    a.boardingProgress = 50.0;
    b.boardingProgress = 50.0005;
    a.plannedFuelKg = 1000.0;
    b.plannedFuelKg = 1000.0005;
    a.plannedZfwKg = 5000.0;
    b.plannedZfwKg = 5000.0005;

    QVERIFY(AreEquivalent(a, b));
}

void IntegratorSnapshotTest::floatDifferenceAboveEpsilonBreaksEquivalence()
{
    const IntegratorSnapshot a;
    IntegratorSnapshot b;

    b.fuelProgress = 0.01;

    QVERIFY(!AreEquivalent(a, b));

    b.fuelProgress = 0.0;
    b.boardingProgress = 0.01;

    QVERIFY(!AreEquivalent(a, b));

    b.boardingProgress = 0.0;
    b.plannedFuelKg = 0.01;

    QVERIFY(!AreEquivalent(a, b));

    b.plannedFuelKg = 0.0;
    b.plannedZfwKg = 0.01;

    QVERIFY(!AreEquivalent(a, b));
}

QTEST_APPLESS_MAIN(IntegratorSnapshotTest)

#include "tst_integrator_snapshot.moc"
