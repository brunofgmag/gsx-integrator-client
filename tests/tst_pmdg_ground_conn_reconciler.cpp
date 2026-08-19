#include <QtTest/QTest>

#include <algorithm>
#include "doubles/FakePmdgTabletGateway.h"
#include "../src/infrastructure/pmdg/PmdgGroundConnReconciler.h"
#include "../src/infrastructure/pmdg/PmdgGroundSource.h"

namespace
{
    constexpr auto kChocksRequest = "wheel_chocks";
    constexpr auto kGroundPowerRequest = "ground_power";

    class FakeGroundSource final : public PmdgGroundSource
    {
    public:
        bool aircraftPowered = false;
        bool groundPowerConnected = false;
        bool groundPowerPresent = false;
        bool chocksSet = false;

        [[nodiscard]] bool HasAircraftPower() const override { return aircraftPowered; }
        [[nodiscard]] bool GroundPowerConnected() const override { return groundPowerConnected; }
        [[nodiscard]] bool GroundPowerPresent() const override { return groundPowerPresent; }
        [[nodiscard]] bool ChocksSet() const override { return chocksSet; }
    };

    int RequestCount(const FakePmdgTabletGateway& tablet, const char* key)
    {
        return static_cast<int>(std::ranges::count(tablet.groundConnRequests, key));
    }
}

class PmdgGroundConnReconcilerTest final : public QObject
{
    Q_OBJECT

private slots:
    static void quietUntilSomethingIsAsked();
    static void chocksAreRequestedUntilTheyAppear();
    static void chocksStopAtTheAttemptCap();
    static void groundPowerCountsAsPresentNotConnected();
    static void groundPowerIsRequestedUntilItAppears();
    static void reversingTheRequestRearmsTheRetry();
};

void PmdgGroundConnReconcilerTest::quietUntilSomethingIsAsked()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundConnRequests.empty());
}

void PmdgGroundConnReconcilerTest::chocksAreRequestedUntilTheyAppear()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);

    source.chocksSet = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::chocksStopAtTheAttemptCap()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 10);
}

void PmdgGroundConnReconcilerTest::groundPowerCountsAsPresentNotConnected()
{
    FakeGroundSource source;
    source.groundPowerPresent = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundConnRequests.empty());
}

void PmdgGroundConnReconcilerTest::groundPowerIsRequestedUntilItAppears()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);

    source.groundPowerPresent = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);
}

void PmdgGroundConnReconcilerTest::reversingTheRequestRearmsTheRetry()
{
    FakeGroundSource source;
    source.chocksSet = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();
    QVERIFY(tablet.groundConnRequests.empty());

    reconciler.SetChocks(false);
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

QTEST_APPLESS_MAIN(PmdgGroundConnReconcilerTest)

#include "tst_pmdg_ground_conn_reconciler.moc"
