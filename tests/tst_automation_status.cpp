#include <QtTest/QTest>

#include "../src/domain/model/AutomationStatus.h"

class AutomationStatusTest final : public QObject
{
    Q_OBJECT

private slots:
    static void resetRestoresDefaults();
};

void AutomationStatusTest::resetRestoresDefaults()
{
    AutomationStatus status;
    status.enabled = true;
    status.aircraftSupported = true;
    status.gsxAvailable = true;
    status.fuelProgress = 80.0;
    status.boardingProgress = 90.0;
    status.deboardingProgress = 64.5;
    status.plannedFuelKg = 12000.0;
    status.loadedFuelKg = 6000.0;
    status.plannedZfwKg = 180000.0;
    status.plannedPassengers = 210;
    status.flightPlanStatus = FlightPlanStatus::Ready;

    status.Reset();

    QCOMPARE(status, AutomationStatus{});
}

QTEST_APPLESS_MAIN(AutomationStatusTest)

#include "tst_automation_status.moc"
