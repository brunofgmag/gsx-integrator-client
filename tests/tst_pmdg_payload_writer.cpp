#include <QtTest/QTest>

#include <memory>
#include "doubles/FakePmdgTabletGateway.h"
#include "doubles/FakeVariableGateway.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/infrastructure/pmdg/PmdgPayloadWriter.h"
#include "../src/infrastructure/simvars/SimVars.h"

using namespace simvars;

namespace
{
    constexpr double kEmptyKg = 45000.0;
    constexpr double kPlannedZfwKg = 60000.0;
    constexpr int kPlannedPax = 100;
    constexpr double kLbsPerKg = 2.20462262185;

    struct WriterFixture
    {
        FakeVariableGateway gateway;
        FakePmdgTabletGateway tablet;
        AutomationStatus status;
        std::unique_ptr<PmdgPayloadWriter> writer;

        explicit WriterFixture(const bool cargoVariant = false)
        {
            gateway.avars[kSimEmptyWeight] = kEmptyKg;
            gateway.avars[kSimFuelTotalKg] = 0.0;
            gateway.avars[kSimTotalWeight] = kEmptyKg;
            status.plannedZfwKg = kPlannedZfwKg;
            status.plannedPassengers = kPlannedPax;
            writer = std::make_unique<PmdgPayloadWriter>(tablet, gateway, &status, cargoVariant);
        }

        void SeeZfw(const double zfwKg)
        {
            gateway.avars[kSimTotalWeight] = zfwKg + gateway.avars[kSimFuelTotalKg];
        }
    };
}

class PmdgPayloadWriterTest final : public QObject
{
    Q_OBJECT

private slots:
    static void fuelIsSentOncePerRoundedValue();
    static void nothingIsSentWhileTheTabletIsAway();
    static void zfwWaitsForTheEmptyWeightToArrive();
    static void passengersAndCargoFollowTheProgress();
    static void cargoVariantSendsNoPassengers();
    static void trimStaysQuietWhileTheProgressiveRampIsMoving();
    static void trimResumesWhenTheProgressiveRampReachesThePlan();
    static void trimWaitsForTheWeightToSettle();
    static void trimCorrectsTheCargoTowardsTheRequest();
    static void trimGivesUpAfterTheAttemptCap();
    static void resetForgetsEverySentValue();
};

void PmdgPayloadWriterTest::fuelIsSentOncePerRoundedValue()
{
    WriterFixture fixture;

    fixture.writer->SetFuelKg(1000.0);
    fixture.writer->SetFuelKg(1000.0);

    QCOMPARE(fixture.tablet.fuelSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet.fuelSends.front(), static_cast<int>(std::lround(1000.0 * kLbsPerKg)));
}

void PmdgPayloadWriterTest::nothingIsSentWhileTheTabletIsAway()
{
    WriterFixture fixture;
    fixture.tablet.available = false;

    fixture.writer->SetFuelKg(1000.0);
    fixture.writer->SetZfwKg(50000.0);

    QVERIFY(fixture.tablet.fuelSends.empty());
    QVERIFY(fixture.tablet.cargoSends.empty());
    QVERIFY(fixture.tablet.paxSends.empty());
}

void PmdgPayloadWriterTest::zfwWaitsForTheEmptyWeightToArrive()
{
    WriterFixture fixture;
    fixture.gateway.avars.erase(kSimEmptyWeight);

    fixture.writer->SetZfwKg(50000.0);

    QVERIFY(fixture.tablet.cargoSends.empty());
}

void PmdgPayloadWriterTest::passengersAndCargoFollowTheProgress()
{
    WriterFixture fixture;

    fixture.writer->SetZfwKg(kEmptyKg + (kPlannedZfwKg - kEmptyKg) / 2.0);

    QCOMPARE(fixture.tablet.paxSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet.paxSends.front(), kPlannedPax / 2);
    QCOMPARE(fixture.tablet.cargoSends.size(), static_cast<std::size_t>(1));

    const double plannedCargoKg = kPlannedZfwKg - kEmptyKg - kPlannedPax * 84.0;
    QCOMPARE(fixture.tablet.cargoSends.front(),
             static_cast<int>(std::lround(0.5 * plannedCargoKg * kLbsPerKg)));
}

void PmdgPayloadWriterTest::cargoVariantSendsNoPassengers()
{
    WriterFixture fixture(true);

    fixture.writer->SetZfwKg(kEmptyKg + (kPlannedZfwKg - kEmptyKg) / 2.0);

    QVERIFY(fixture.tablet.paxSends.empty());
    QCOMPARE(fixture.tablet.cargoSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet.cargoSends.front(),
             static_cast<int>(std::lround(0.5 * (kPlannedZfwKg - kEmptyKg) * kLbsPerKg)));
}

void PmdgPayloadWriterTest::trimStaysQuietWhileTheProgressiveRampIsMoving()
{
    WriterFixture fixture;
    const double spanKg = kPlannedZfwKg - kEmptyKg;

    for (int step = 1; step <= 4; ++step)
    {
        const double requestedKg = kEmptyKg + spanKg * step / 10.0;
        fixture.writer->SetZfwKg(requestedKg);
        fixture.SeeZfw(requestedKg + 1000.0);
        fixture.writer->Trim();
    }

    const std::size_t sendsWhenTheRampPaused = fixture.tablet.cargoSends.size();

    for (int tick = 0; tick < 6; ++tick)
    {
        fixture.writer->Trim();
    }

    QCOMPARE(fixture.tablet.cargoSends.size(), sendsWhenTheRampPaused);
}

void PmdgPayloadWriterTest::trimResumesWhenTheProgressiveRampReachesThePlan()
{
    WriterFixture fixture;

    fixture.writer->SetZfwKg(kEmptyKg + (kPlannedZfwKg - kEmptyKg) / 2.0);
    fixture.SeeZfw(kPlannedZfwKg + 500.0);

    for (int tick = 0; tick < 10; ++tick)
    {
        fixture.writer->Trim();
    }

    fixture.writer->SetZfwKg(kPlannedZfwKg);
    const int requestedLbs = fixture.tablet.cargoSends.back();

    for (int tick = 0; tick < 5; ++tick)
    {
        fixture.writer->Trim();
    }

    QCOMPARE(fixture.tablet.cargoSends.back(),
             requestedLbs - static_cast<int>(std::lround(500.0 * kLbsPerKg)));
}

void PmdgPayloadWriterTest::trimWaitsForTheWeightToSettle()
{
    WriterFixture fixture;
    fixture.writer->SetZfwKg(kPlannedZfwKg);
    fixture.SeeZfw(kPlannedZfwKg - 500.0);
    const std::size_t sendsAfterRequest = fixture.tablet.cargoSends.size();

    for (int tick = 0; tick < 4; ++tick)
    {
        fixture.writer->Trim();
    }

    QCOMPARE(fixture.tablet.cargoSends.size(), sendsAfterRequest);

    fixture.writer->Trim();

    QCOMPARE(fixture.tablet.cargoSends.size(), sendsAfterRequest + 1);
}

void PmdgPayloadWriterTest::trimCorrectsTheCargoTowardsTheRequest()
{
    WriterFixture fixture;
    fixture.writer->SetZfwKg(kPlannedZfwKg);
    const int requestedLbs = fixture.tablet.cargoSends.back();
    fixture.SeeZfw(kPlannedZfwKg + 500.0);

    for (int tick = 0; tick < 5; ++tick)
    {
        fixture.writer->Trim();
    }

    QCOMPARE(fixture.tablet.cargoSends.back(),
             requestedLbs - static_cast<int>(std::lround(500.0 * kLbsPerKg)));
}

void PmdgPayloadWriterTest::trimGivesUpAfterTheAttemptCap()
{
    WriterFixture fixture;
    fixture.writer->SetZfwKg(kPlannedZfwKg);

    for (int tick = 0; tick < 200; ++tick)
    {
        fixture.SeeZfw(kPlannedZfwKg + 500.0);
        fixture.writer->Trim();
    }

    QCOMPARE(fixture.tablet.cargoSends.size(), static_cast<std::size_t>(6));
}

void PmdgPayloadWriterTest::resetForgetsEverySentValue()
{
    WriterFixture fixture;
    fixture.writer->SetFuelKg(1000.0);

    fixture.writer->Reset();
    fixture.writer->SetFuelKg(1000.0);

    QCOMPARE(fixture.tablet.fuelSends.size(), static_cast<std::size_t>(2));
}

QTEST_APPLESS_MAIN(PmdgPayloadWriterTest)

#include "tst_pmdg_payload_writer.moc"
