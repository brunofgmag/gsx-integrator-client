#include <QtTest/QTest>

#include <algorithm>
#include <memory>
#include "doubles/FakePmdg777DataGateway.h"
#include "doubles/FakePmdg777TabletGateway.h"
#include "doubles/FakeVariableGateway.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/infrastructure/aircraft/Pmdg777.h"

namespace
{
    constexpr auto kAvionicsPoweredLVar = "PowerOn";
    constexpr auto kSmartSwitchCaptLVar = "switch_554_a";
    constexpr auto kSmartSwitchFoLVar = "switch_773_a";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";

    struct Pmdg777Fixture
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        FakePmdg777DataGateway* data = nullptr;
        FakePmdg777TabletGateway* tablet = nullptr;
        std::unique_ptr<Pmdg777> aircraft;

        explicit Pmdg777Fixture(const Pmdg777Variant variant = Pmdg777Variant::Er300)
        {
            auto dataGateway = std::make_unique<FakePmdg777DataGateway>();
            auto tabletGateway = std::make_unique<FakePmdg777TabletGateway>();
            data = dataGateway.get();
            tablet = tabletGateway.get();
            aircraft = std::make_unique<Pmdg777>(&gateway, &status, variant,
                                                 std::move(dataGateway), std::move(tabletGateway));
        }

        void SeedEnginesOff()
        {
            gateway.avars[kSimEng1Combustion] = 0.0;
            gateway.avars[kSimEng2Combustion] = 0.0;
        }

        void SeedSecuredOnStand()
        {
            SeedEnginesOff();
            data->hasData = true;
            data->parkingBrakeOn = true;
        }

        void SeedWeights(const double emptyKg, const double plannedZfwKg, const int plannedPax = 0)
        {
            gateway.avars["EMPTY WEIGHT"] = emptyKg;
            status.plannedZfwKg = plannedZfwKg;
            status.plannedPassengers = plannedPax;
        }
    };
}

class Pmdg777Test final : public QObject
{
    Q_OBJECT

private slots:
    static void groundPowerUnknownUntilData();
    static void groundPowerFollowsExtPowerAnnunciator();
    static void onTickPollsGateways();
    static void notPoweredWhileNothingReceived();
    static void notPoweredByTabletLvarWhileDark();
    static void poweredByApuOrExtPower();
    static void poweredByRunningEngine();
    static void engineRunningConservativeUntilReceived();
    static void parkingBrakeRequiresClientData();
    static void smartSwitchEdgesOncePerPress();
    static void smartSwitchWorksFromBothSeats();
    static void smartSwitchCatchesTransientPress();
    static void smartSwitchLVarsGetFastRefresh();
    static void readyToPushMatrix();
    static void readyToDeboardMatrix();
    static void flightPlanWaitsForFmcPlan();
    static void setFuelSendsRoundedLbs();
    static void setFuelDedupsRepeatedValues();
    static void setFuelHeldWhileTabletUnavailable();
    static void loadingStartResetsFuelDedup();
    static void setZfwFreighterSendsCargoOnly();
    static void setZfwPaxVariantSplitsPaxAndCargo();
    static void setZfwClampsToPayloadSpan();
    static void setZfwDedupsAndDrainsOnDeboard();
    static void setZfwHeldUntilEmptyWeightReceived();
    static void doorsFollowGsxLoaders();
    static void doorsTakeOverGsxDoorAutomation();
    static void doorsSkipTogglesWhileMoving();
    static void doorsHoldBeforeClientData();
    static void closeAllDoorsTogglesOpenMappedDoors();
    static void chocksReconcileWithRetryCap();
    static void parsesOptionsDataBroadcastFlag();
    static void zfwTrimsCargoAgainstActualWeight();
    static void jetwayDoorClosesAtItsOpenedIndex();
    static void aftCateringDoorOpensFiveRightOnlyOn300();
    static void groundPowerConnectFlow();
    static void groundPowerDisconnectFlow();
};

void Pmdg777Test::groundPowerUnknownUntilData()
{
    const Pmdg777Fixture fixture;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus().value(), GroundPowerStatus::Unknown);
}

void Pmdg777Test::groundPowerFollowsExtPowerAnnunciator()
{
    const Pmdg777Fixture fixture;

    fixture.data->hasData = true;
    fixture.data->extPowerConnected = true;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus().value(), GroundPowerStatus::Connected);

    fixture.data->extPowerConnected = false;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus().value(), GroundPowerStatus::Disconnected);
}

void Pmdg777Test::onTickPollsGateways()
{
    const Pmdg777Fixture fixture;

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.data->pollCalls, 1);
    QCOMPARE(fixture.tablet->pollCalls, 1);
    QVERIFY(!fixture.data->inFlight);
}

void Pmdg777Test::notPoweredWhileNothingReceived()
{
    const Pmdg777Fixture fixture;

    QVERIFY(!fixture.aircraft->IsPowered());
}

void Pmdg777Test::notPoweredByTabletLvarWhileDark()
{
    Pmdg777Fixture fixture;

    fixture.gateway.lvars[kAvionicsPoweredLVar] = 1.0;
    fixture.data->hasData = true;

    QVERIFY(!fixture.aircraft->IsPowered());
}

void Pmdg777Test::poweredByApuOrExtPower()
{
    const Pmdg777Fixture fixture;

    fixture.data->hasData = true;
    fixture.data->apuRunning = true;

    QVERIFY(fixture.aircraft->IsPowered());

    fixture.data->apuRunning = false;

    QVERIFY(!fixture.aircraft->IsPowered());

    fixture.data->extPowerConnected = true;

    QVERIFY(fixture.aircraft->IsPowered());
}

void Pmdg777Test::poweredByRunningEngine()
{
    Pmdg777Fixture fixture;

    fixture.SeedEnginesOff();
    fixture.gateway.avars[kSimEng2Combustion] = 1.0;

    QVERIFY(fixture.aircraft->IsPowered());
}

void Pmdg777Test::engineRunningConservativeUntilReceived()
{
    Pmdg777Fixture fixture;

    QVERIFY(fixture.aircraft->IsEngineRunning());

    fixture.SeedEnginesOff();

    QVERIFY(!fixture.aircraft->IsEngineRunning());

    fixture.gateway.avars[kSimEng1Combustion] = 1.0;

    QVERIFY(fixture.aircraft->IsEngineRunning());
}

void Pmdg777Test::parkingBrakeRequiresClientData()
{
    const Pmdg777Fixture fixture;

    fixture.data->parkingBrakeOn = true;

    QVERIFY(!fixture.aircraft->IsParkingBrakeSet());

    fixture.data->hasData = true;

    QVERIFY(fixture.aircraft->IsParkingBrakeSet());
}

void Pmdg777Test::smartSwitchEdgesOncePerPress()
{
    Pmdg777Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->OnTick();

    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvars[kSmartSwitchCaptLVar] = 100.0;

    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvars[kSmartSwitchCaptLVar] = 0.0;

    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvars[kSmartSwitchCaptLVar] = 100.0;

    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg777Test::smartSwitchWorksFromBothSeats()
{
    Pmdg777Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->OnTick();
    fixture.gateway.lvars[kSmartSwitchFoLVar] = 100.0;

    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg777Test::smartSwitchCatchesTransientPress()
{
    Pmdg777Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->OnTick();
    fixture.gateway.lvars[kSmartSwitchCaptLVar] = 50.0;
    fixture.gateway.lvarSpans[kSmartSwitchCaptLVar] = LVarSpan{50.0, 100.0, true};

    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg777Test::smartSwitchLVarsGetFastRefresh()
{
    Pmdg777Fixture fixture;

    fixture.aircraft->OnTick();

    QVERIFY(fixture.gateway.fastRefreshNames.empty());

    fixture.gateway.lvars[kSmartSwitchCaptLVar] = 100.0;

    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.data->hasData = true;
    fixture.aircraft->OnTick();

    const auto& names = fixture.gateway.fastRefreshNames;

    QVERIFY(std::ranges::find(names, kSmartSwitchCaptLVar) != names.end());
    QVERIFY(std::ranges::find(names, kSmartSwitchFoLVar) != names.end());
}

void Pmdg777Test::readyToPushMatrix()
{
    Pmdg777Fixture fixture;

    fixture.SeedEnginesOff();
    fixture.data->hasData = true;
    fixture.data->extPowerConnected = true;
    fixture.data->beaconOn = true;

    QVERIFY(fixture.aircraft->IsReadyToPush());

    fixture.data->beaconOn = false;

    QVERIFY(!fixture.aircraft->IsReadyToPush());

    fixture.data->beaconOn = true;
    fixture.gateway.avars[kSimEng1Combustion] = 1.0;

    QVERIFY(!fixture.aircraft->IsReadyToPush());

    fixture.gateway.avars[kSimEng1Combustion] = 0.0;
    fixture.data->extPowerConnected = false;

    QVERIFY(!fixture.aircraft->IsReadyToPush());
}

void Pmdg777Test::readyToDeboardMatrix()
{
    Pmdg777Fixture fixture;

    fixture.SeedSecuredOnStand();

    QVERIFY(fixture.aircraft->IsReadyToDeboard());

    fixture.data->beaconOn = true;

    QVERIFY(!fixture.aircraft->IsReadyToDeboard());

    fixture.data->beaconOn = false;
    fixture.data->parkingBrakeOn = false;

    QVERIFY(!fixture.aircraft->IsReadyToDeboard());

    fixture.data->wheelChocksSet = true;

    QVERIFY(fixture.aircraft->IsReadyToDeboard());

    fixture.gateway.avars[kSimEng1Combustion] = 1.0;

    QVERIFY(!fixture.aircraft->IsReadyToDeboard());
}

void Pmdg777Test::flightPlanWaitsForFmcPlan()
{
    Pmdg777Fixture fixture;

    fixture.status.flightPlanStatus = FlightPlanStatus::Ready;
    fixture.data->hasData = true;

    QVERIFY(fixture.aircraft->RequiresEfbFlightPlan());
    QVERIFY(!fixture.aircraft->IsFlightPlanLoaded());

    fixture.data->hasFmcFlightPlan = true;

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());

    fixture.data->hasFmcFlightPlan = false;
    fixture.tablet->efbPlanImported = true;

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());

    fixture.status.flightPlanStatus = FlightPlanStatus::Idle;

    QVERIFY(!fixture.aircraft->IsFlightPlanLoaded());
}

void Pmdg777Test::setFuelSendsRoundedLbs()
{
    const Pmdg777Fixture fixture;

    fixture.aircraft->SetCurrentFuelKg(10000.0);
    fixture.aircraft->SetCurrentFuelKg(9000.0);

    QCOMPARE(fixture.tablet->fuelSends.size(), static_cast<std::size_t>(2));
    QCOMPARE(fixture.tablet->fuelSends[0], 22046);
    QCOMPARE(fixture.tablet->fuelSends[1], 19842);
}

void Pmdg777Test::setFuelDedupsRepeatedValues()
{
    const Pmdg777Fixture fixture;

    fixture.aircraft->SetCurrentFuelKg(10000.0);
    fixture.aircraft->SetCurrentFuelKg(10000.1);
    fixture.aircraft->SetCurrentFuelKg(10001.0);

    QCOMPARE(fixture.tablet->fuelSends.size(), static_cast<std::size_t>(2));
}

void Pmdg777Test::setFuelHeldWhileTabletUnavailable()
{
    const Pmdg777Fixture fixture;

    fixture.tablet->available = false;
    fixture.aircraft->SetCurrentFuelKg(10000.0);

    QVERIFY(fixture.tablet->fuelSends.empty());

    fixture.tablet->available = true;
    fixture.aircraft->SetCurrentFuelKg(10000.0);

    QCOMPARE(fixture.tablet->fuelSends.size(), static_cast<std::size_t>(1));
}

void Pmdg777Test::loadingStartResetsFuelDedup()
{
    const Pmdg777Fixture fixture;

    fixture.aircraft->SetCurrentFuelKg(10000.0);
    fixture.aircraft->OnLoadingStarted();
    fixture.aircraft->SetCurrentFuelKg(10000.0);

    QCOMPARE(fixture.tablet->fuelSends.size(), static_cast<std::size_t>(2));
}

void Pmdg777Test::setZfwFreighterSendsCargoOnly()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.SeedWeights(144000.0, 244000.0, 4);
    fixture.aircraft->SetCurrentZfwKg(194000.0);

    QVERIFY(fixture.tablet->paxSends.empty());
    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet->cargoSends[0], 110231);
}

void Pmdg777Test::setZfwPaxVariantSplitsPaxAndCargo()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Er300);

    fixture.SeedWeights(160000.0, 200000.0, 300);
    fixture.aircraft->SetCurrentZfwKg(180000.0);

    QCOMPARE(fixture.tablet->paxSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet->paxSends[0], 150);
    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet->cargoSends[0], 16314);
}

void Pmdg777Test::setZfwClampsToPayloadSpan()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.SeedWeights(144000.0, 244000.0);
    fixture.aircraft->SetCurrentZfwKg(300000.0);

    QCOMPARE(fixture.tablet->cargoSends.back(), 220462);

    fixture.aircraft->SetCurrentZfwKg(100000.0);

    QCOMPARE(fixture.tablet->cargoSends.back(), 0);
}

void Pmdg777Test::setZfwDedupsAndDrainsOnDeboard()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.SeedWeights(144000.0, 244000.0);
    fixture.aircraft->SetCurrentZfwKg(194000.0);
    fixture.aircraft->SetCurrentZfwKg(194000.0);

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));

    fixture.aircraft->SetCurrentZfwKg(169000.0);

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));
    QVERIFY(fixture.tablet->cargoSends[1] < fixture.tablet->cargoSends[0]);
}

void Pmdg777Test::setZfwHeldUntilEmptyWeightReceived()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.status.plannedZfwKg = 244000.0;
    fixture.aircraft->SetCurrentZfwKg(194000.0);

    QVERIFY(fixture.tablet->cargoSends.empty());
}

void Pmdg777Test::doorsFollowGsxLoaders()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;
    fixture.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    fixture.gateway.lvars["FSDT_GSX_VEHICLE_BAGGAGELOADERFRONT_STATE"] = 8.0;
    fixture.gateway.lvars["FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE"] = 8.0;

    fixture.aircraft->OnTick();

    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 10) != fixture.data->toggledDoors.end());
    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 12) != fixture.data->toggledDoors.end());
    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 11) == fixture.data->toggledDoors.end());

    fixture.data->doorStates[10] = 0;
    fixture.data->doorStates[12] = 0;
    fixture.data->toggledDoors.clear();
    fixture.aircraft->OnTick();

    QVERIFY(fixture.data->toggledDoors.empty());
}

void Pmdg777Test::doorsSkipTogglesWhileMoving()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;
    fixture.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    fixture.gateway.lvars["FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE"] = 8.0;
    fixture.data->doorStates[12] = 4;

    fixture.aircraft->OnTick();

    QVERIFY(fixture.data->toggledDoors.empty());

    fixture.data->doorStates[12] = 1;
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.data->toggledDoors.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.data->toggledDoors[0], 12);
}

void Pmdg777Test::doorsTakeOverGsxDoorAutomation()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.gateway.Written("FSDT_GSX_AUTOMATION_DOORS"), 0.0);

    fixture.gateway.lvars["FSDT_GSX_AUTOMATION_DOORS"] = 1.0;
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.gateway.Written("FSDT_GSX_AUTOMATION_DOORS"), 0.0);
}

void Pmdg777Test::doorsHoldBeforeClientData()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    fixture.gateway.lvars["FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE"] = 8.0;

    fixture.aircraft->OnTick();

    QVERIFY(fixture.data->toggledDoors.empty());
}

void Pmdg777Test::closeAllDoorsTogglesOpenMappedDoors()
{
    const Pmdg777Fixture fixture(Pmdg777Variant::Freighter);
    
    fixture.data->hasData = true;
    fixture.data->doorStates[10] = 0;
    fixture.data->doorStates[12] = 0;
    fixture.data->doorStates[11] = 1;

    fixture.aircraft->CloseAllDoors();

    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 10) != fixture.data->toggledDoors.end());
    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 12) != fixture.data->toggledDoors.end());
    QVERIFY(std::ranges::find(fixture.data->toggledDoors, 11) == fixture.data->toggledDoors.end());
}

void Pmdg777Test::chocksReconcileWithRetryCap()
{
    const Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;

    fixture.aircraft->SetChocks(true);
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("wheel_chocks"));

    for (int tick = 0; tick < 4; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));

    fixture.data->wheelChocksSet = true;
    for (int tick = 0; tick < 10; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));

    for (int tick = 0; tick < 200; ++tick)
    {
        fixture.data->wheelChocksSet = false;
        fixture.aircraft->OnTick();
    }
    QVERIFY(fixture.tablet->groundConnRequests.size() <= static_cast<std::size_t>(12));
}

void Pmdg777Test::groundPowerConnectFlow()
{
    const Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;

    fixture.aircraft->SetGroundPower(true);
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("ground_power"));

    for (int tick = 0; tick < 4; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));

    fixture.data->extPowerAvailable = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
}

void Pmdg777Test::groundPowerDisconnectFlow()
{
    const Pmdg777Fixture fixture(Pmdg777Variant::Freighter);

    fixture.data->hasData = true;
    fixture.data->extPowerConnected = true;

    fixture.aircraft->SetGroundPower(false);
    fixture.aircraft->OnTick();
    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("ground_power"));

    fixture.data->extPowerConnected = false;
    for (int tick = 0; tick < 20; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
}

void Pmdg777Test::parsesOptionsDataBroadcastFlag()
{
    QVERIFY(Pmdg777::OptionsEnableDataBroadcast("[SDK]\r\nEnableDataBroadcast=1\r\n"));
    QVERIFY(Pmdg777::OptionsEnableDataBroadcast("[Misc]\nFoo=2\n[SDK]\nEnableDataBroadcast = 1\n"));
    QVERIFY(!Pmdg777::OptionsEnableDataBroadcast("[SDK]\nEnableDataBroadcast=0\n"));
    QVERIFY(!Pmdg777::OptionsEnableDataBroadcast("[Misc]\nEnableDataBroadcast=1\n"));
    QVERIFY(!Pmdg777::OptionsEnableDataBroadcast(""));
}

void Pmdg777Test::zfwTrimsCargoAgainstActualWeight()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Er300);

    fixture.data->hasData = true;
    fixture.SeedWeights(160000.0, 200000.0, 300);
    fixture.gateway.avars["FUEL TOTAL QUANTITY WEIGHT"] = 0.0;
    fixture.gateway.avars["TOTAL WEIGHT"] = 201200.0;

    fixture.aircraft->SetCurrentZfwKg(200000.0);

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));

    const int initialCargo = fixture.tablet->cargoSends[0];

    for (int tick = 0; tick < 4; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));
    QCOMPARE(fixture.tablet->cargoSends[1], initialCargo - 2646);

    for (int tick = 0; tick < 100; ++tick)
    {
        fixture.aircraft->OnTick();
    }
    QVERIFY(fixture.tablet->cargoSends.size() <= static_cast<std::size_t>(6));
}

void Pmdg777Test::jetwayDoorClosesAtItsOpenedIndex()
{
    Pmdg777Fixture fixture(Pmdg777Variant::Er300);

    fixture.data->hasData = true;
    fixture.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    fixture.gateway.lvars["FSDT_GSX_JETWAY"] = 5.0;

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.data->toggledDoors, std::vector{2});

    fixture.data->doorStates[2] = 0;
    fixture.gateway.lvars["FSDT_GSX_JETWAY"] = 2.0;
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.data->toggledDoors, (std::vector{2, 2}));
}

void Pmdg777Test::aftCateringDoorOpensFiveRightOnlyOn300()
{
    Pmdg777Fixture er300(Pmdg777Variant::Er300);
    er300.data->hasData = true;
    er300.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    er300.gateway.lvars["FSDT_GSX_VEHICLE_CATERINGVEHICLEREAR_STATE"] = 7.0;

    er300.aircraft->OnTick();

    QVERIFY(std::ranges::find(er300.data->toggledDoors, 9) != er300.data->toggledDoors.end());
    QVERIFY(std::ranges::find(er300.data->toggledDoors, 7) == er300.data->toggledDoors.end());

    Pmdg777Fixture er200(Pmdg777Variant::Er200);
    er200.data->hasData = true;
    er200.gateway.lvars["FSDT_GSX_COUATL_STARTED"] = 1.0;
    er200.gateway.lvars["FSDT_GSX_VEHICLE_CATERINGVEHICLEREAR_STATE"] = 7.0;

    er200.aircraft->OnTick();

    QVERIFY(std::ranges::find(er200.data->toggledDoors, 7) != er200.data->toggledDoors.end());
    QVERIFY(std::ranges::find(er200.data->toggledDoors, 9) == er200.data->toggledDoors.end());
}

QTEST_APPLESS_MAIN(Pmdg777Test)

#include "tst_pmdg777.moc"
