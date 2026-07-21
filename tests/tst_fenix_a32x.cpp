#include <QtTest/QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "TestDoubles.h"
#include "../src/infrastructure/aircraft/FenixA32x.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";

    constexpr auto kSmartSwitch = "S_ASP_INTRAD";
    constexpr auto kParkingBrakeLvar = "S_MIP_PARKING_BRAKE";
    constexpr auto kDcEssBus = "B_ELEC_BUS_POWER_DC_ESS";
    constexpr auto kBattery1 = "S_OH_ELEC_BAT1";
    constexpr auto kBattery2 = "S_OH_ELEC_BAT2";
    constexpr auto kExtPowerOnBus = "I_OH_ELEC_EXT_PWR_L";
    constexpr auto kApuRunning = "I_OH_ELEC_APU_START_U";
    constexpr auto kGpuPlaced = "B_CONFIG_GPU";
    constexpr auto kChocks = "B_CONFIG_CHOCKS";
    constexpr auto kThirdPartyRefuel = "S_THIRD_PARTY_REFUELG";

    constexpr auto kChocksDataref = "fenix.efb.chocks";
    constexpr auto kGroundPowerDataref = "groundservice.groundpower";

    constexpr auto kWeightUnitDataref = "system.config.Units.Weight";
    constexpr auto kFuelDataref = "aircraft.fuel.total.amount.kg";
    constexpr auto kFuelTargetDataref = "aircraft.refuel.fuelTarget.kg";
    constexpr auto kCargoTargetDataref = "fenix.efb.plannedCargoKg";
    constexpr auto kSimbriefImported = "fenix.efb.simbriefPlanImported";
    constexpr auto kBookedSeats = "fenix.efb.passengers.booked";
    constexpr auto kSeatOccupationString = "aircraft.passengers.seatOccupation.string";
    constexpr auto kFwdCargo = "aircraft.cargo.forward.amount";
    constexpr auto kAftCargo = "aircraft.cargo.aft.amount";
    constexpr auto kBulkCargo = "aircraft.cargo.bulk.amount";

    constexpr auto kFwdPaxDoor = "doors.entry.d1l";
    constexpr auto kMidPaxDoor = "doors.entry.d2l";
    constexpr auto kAftPaxDoor = "doors.entry.d4l";
    constexpr auto kFwdCateringDoor = "doors.entry.d1r";
    constexpr auto kAftCateringDoor = "doors.entry.d4r";
    constexpr auto kFwdCargoDoor = "doors.cargo.forward";
    constexpr auto kAftCargoDoor = "doors.cargo.aft";

    constexpr double kEmptyWeightKg = 40000.0;
    constexpr double kPlannedZfwKg = 60000.0;
    constexpr double kPlannedFuelKg = 9500.0;
    constexpr int kPlannedPassengers = 150;
    constexpr int kSeatCapacity = 180;
    constexpr double kPlannedCargoKg = 20000.0 - kPlannedPassengers * 84.0;
    constexpr double kGsxStateActive = 5.0;
    constexpr double kGsxStateCompleted = 6.0;

    struct FenixFixture
    {
        explicit FenixFixture(const FenixVariant variant = FenixVariant::A320)
            : aircraft{&gateway, variant, std::move(efbOwner)}
        {
        }

        FakeVariableGateway gateway;
        std::unique_ptr<FakeFenixEfbGateway> efbOwner = std::make_unique<FakeFenixEfbGateway>();
        FakeFenixEfbGateway& efb = *efbOwner;
        FenixA32x aircraft;

        void SeedBookedSeats() const
        {
            std::vector<bool> booked(kSeatCapacity, false);
            std::fill_n(booked.begin(), kPlannedPassengers, true);
            efb.boolArrays[kBookedSeats] = booked;
        }

        void SeedPlannedLoad()
        {
            gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
            SeedBookedSeats();
            efb.numbers[kCargoTargetDataref] = kPlannedCargoKg;
        }

        void SeedFlightPlanLoaded()
        {
            gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
            efb.numbers[kSimbriefImported] = 1.0;
            efb.numbers[kFuelTargetDataref] = kPlannedFuelKg;
        }
    };

    int CountOccupiedSeats(const std::string& seatString)
    {
        int count = 0;
        std::size_t pos = 0;
        while ((pos = seatString.find("true", pos)) != std::string::npos)
        {
            ++count;
            pos += 4;
        }

        return count;
    }

    int CountSeats(const std::string& seatString)
    {
        if (seatString.empty())
        {
            return 0;
        }

        return static_cast<int>(std::ranges::count(seatString, ',')) + 1;
    }
}

class FenixA32xTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsNamePerVariant();
    static void reportsLoadMethodsAndCapabilities();
    static void registersSmartSwitchForFastRefresh();
    static void subscribesEfbPlanDatarefs();
    static void pollsEfbEveryTick();
    static void disablesEfbAutomationOnceAvailable();
    static void reinitializesEfbAutomationAfterOutage();
    static void leavesEfbUntouchedWhileUnavailable();
    static void readsCurrentFuelFromSim();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwDoesNotDropBelowEmptyWeight();
    static void emptyZfwReadsSimEmptyWeight();
    static void readsNativeWeightUnitFromEfb();
    static void plannedValuesComeFromEfb();
    static void flightPlanNeedsEfbImportAndFuelTarget();
    static void loadingStartArmsThirdPartyRefueling();
    static void thirdPartyRefuelingDisarmsWhenGsxCompletes();
    static void fuelSetterWritesEfbOncePerValue();
    static void fuelSetterSilentWhileEfbUnavailable();
    static void zfwSetterBoardsSeatsAndCargoProgressively();
    static void zfwSetterSnapsToPlannedOnCompletion();
    static void zfwSetterDrainsToEmptyOnDeboard();
    static void zfwSetterIgnoresRepeatedValues();
    static void zfwSetterSilentWhileEfbUnavailable();
    static void zfwSetterWaitsForEmptyWeightData();
    static void zfwSetterSkipsWithoutPlannedZfw();
    static void seatWritesWaitForBookedSeats();
    static void preliminaryLoadsheetRequestedWhenLoadingStarts();
    static void preliminaryLoadsheetSkippedWhileEfbUnavailable();
    static void finalLoadsheetRequestedOnceAtPlannedZfw();
    static void finalLoadsheetWaitsForLoadingStart();
    static void cargoDoorsFollowBaggageLoaders();
    static void paxDoorOpensForJetwayOrFrontStairs();
    static void midPaxDoorDrivenOnlyOnA321();
    static void cateringDoorsFollowCateringVehicles();
    static void doorsUntouchedWithoutGsx();
    static void doorsUntouchedWhileEfbUnavailable();
    static void closeAllDoorsForcesEveryDoorClosed();
    static void stairsReopenDoorAfterCloseAllDoors();
    static void groundPowerWritesEfbDataref();
    static void groundPowerStatusFollowsGpuAndExtPower();
    static void chocksWriteEfbDataref();
    static void smartSwitchFiresOnceAndResetsTheSwitch();
    static void smartSwitchIgnoresRadioPosition();
    static void smartSwitchStaysQuietUntilDataArrives();
    static void powerRequiresBatteryPlusExternalOrApu();
    static void engineAssumedRunningUntilDataArrives();
    static void engineRunningDetectsEitherEngine();
    static void parkingBrakeRequiresBothSources();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
};

void FenixA32xTest::reportsNamePerVariant()
{
    const FenixFixture a319{FenixVariant::A319};
    const FenixFixture a320{FenixVariant::A320};
    const FenixFixture a321{FenixVariant::A321};

    QCOMPARE(QString(a319.aircraft.GetName()), QString("Fenix A319"));
    QCOMPARE(QString(a320.aircraft.GetName()), QString("Fenix A320"));
    QCOMPARE(QString(a321.aircraft.GetName()), QString("Fenix A321"));
    QVERIFY(!a320.aircraft.IsCargoVariant());
}

void FenixA32xTest::reportsLoadMethodsAndCapabilities()
{
    const FenixFixture fixture;

    QVERIFY(fixture.aircraft.GetRefuelMethod() == RefuelBy::Client);
    QVERIFY(fixture.aircraft.GetBoardMethod() == BoardBy::Client);
    QVERIFY(fixture.aircraft.SupportsStairsOrJetways());
    QVERIFY(!fixture.aircraft.CompletesPushbackViaInterruptMenu());
    QVERIFY(fixture.aircraft.SupportsChocksControl());
    QVERIFY(fixture.aircraft.SupportsGroundPowerControl());
    QVERIFY(fixture.aircraft.RequiresEfbFlightPlan());
}

void FenixA32xTest::registersSmartSwitchForFastRefresh()
{
    const FenixFixture fixture;

    QVERIFY(std::ranges::find(fixture.gateway.fastRefreshNames, std::string(kSmartSwitch))
        != fixture.gateway.fastRefreshNames.end());
}

void FenixA32xTest::subscribesEfbPlanDatarefs()
{
    const FenixFixture fixture;

    QVERIFY(std::ranges::find(fixture.efb.subscribed, std::string(kSimbriefImported))
        != fixture.efb.subscribed.end());
    QVERIFY(std::ranges::find(fixture.efb.subscribed, std::string(kBookedSeats))
        != fixture.efb.subscribed.end());
    QVERIFY(std::ranges::find(fixture.efb.subscribed, std::string(kFuelTargetDataref))
        != fixture.efb.subscribed.end());
    QVERIFY(std::ranges::find(fixture.efb.subscribed, std::string(kCargoTargetDataref))
        != fixture.efb.subscribed.end());
    QVERIFY(std::ranges::find(fixture.efb.subscribed, std::string(kWeightUnitDataref))
        != fixture.efb.subscribed.end());
}

void FenixA32xTest::pollsEfbEveryTick()
{
    FenixFixture fixture;

    fixture.aircraft.OnTick();
    fixture.aircraft.OnTick();
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.pollCalls, 3);
}

void FenixA32xTest::disablesEfbAutomationOnceAvailable()
{
    FenixFixture fixture;

    fixture.aircraft.OnTick();
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.setBoolCalls, 8);
    QCOMPARE(fixture.efb.WrittenBool("fenix.efb.autoDoor"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.efb.autoJetway"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoConnectGpu"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoDisconnectGpu"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoDeboard"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoCatering"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoPushback"), 0);
    QCOMPARE(fixture.efb.WrittenBool("fenix.gsx.autoSelectOperator"), 0);
}

void FenixA32xTest::reinitializesEfbAutomationAfterOutage()
{
    FenixFixture fixture;

    fixture.aircraft.OnTick();
    fixture.efb.available = false;
    fixture.aircraft.OnTick();
    fixture.efb.available = true;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.setBoolCalls, 16);
}

void FenixA32xTest::leavesEfbUntouchedWhileUnavailable()
{
    FenixFixture fixture;

    fixture.efb.available = false;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.setBoolCalls, 0);
    QCOMPARE(fixture.efb.setFloatCalls, 0);
    QCOMPARE(fixture.efb.setStringCalls, 0);
}

void FenixA32xTest::readsCurrentFuelFromSim()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimFuelTotalKg] = 8450.0;

    QCOMPARE(fixture.aircraft.GetCurrentFuelKg(), 8450.0);
}

void FenixA32xTest::currentZfwSubtractsFuelFromTotalWeight()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    fixture.gateway.avars[kSimTotalWeight] = 62000.0;
    fixture.gateway.avars[kSimFuelTotalKg] = 8000.0;

    QCOMPARE(fixture.aircraft.GetCurrentZfwKg(), 54000.0);
}

void FenixA32xTest::currentZfwDoesNotDropBelowEmptyWeight()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    fixture.gateway.avars[kSimTotalWeight] = 41000.0;
    fixture.gateway.avars[kSimFuelTotalKg] = 8000.0;

    QCOMPARE(fixture.aircraft.GetCurrentZfwKg(), kEmptyWeightKg);
}

void FenixA32xTest::emptyZfwReadsSimEmptyWeight()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;

    QCOMPARE(fixture.aircraft.GetEmptyZfwKg(), kEmptyWeightKg);
}

void FenixA32xTest::readsNativeWeightUnitFromEfb()
{
    const FenixFixture fixture;

    QVERIFY(!fixture.aircraft.GetNativeWeightUnit().has_value());

    fixture.efb.stringValues[kWeightUnitDataref] = "KG";
    QVERIFY(fixture.aircraft.GetNativeWeightUnit() == WeightUnit::Kg);

    fixture.efb.stringValues[kWeightUnitDataref] = "LBS";
    QVERIFY(fixture.aircraft.GetNativeWeightUnit() == WeightUnit::Lb);

    fixture.efb.stringValues[kWeightUnitDataref] = "lb";
    QVERIFY(fixture.aircraft.GetNativeWeightUnit() == WeightUnit::Lb);

    fixture.efb.stringValues[kWeightUnitDataref] = "tonnes";
    QVERIFY(!fixture.aircraft.GetNativeWeightUnit().has_value());
}

void FenixA32xTest::plannedValuesComeFromEfb()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();
    fixture.efb.numbers[kFuelTargetDataref] = kPlannedFuelKg;

    QCOMPARE(fixture.aircraft.GetPlannedFuelKg(), kPlannedFuelKg);
    QCOMPARE(fixture.aircraft.GetPlannedZfwKg(), kPlannedZfwKg);
    QCOMPARE(fixture.aircraft.GetPlannedPassengers(), kPlannedPassengers);
}

void FenixA32xTest::flightPlanNeedsEfbImportAndFuelTarget()
{
    FenixFixture fixture;

    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());

    fixture.efb.numbers[kSimbriefImported] = 1.0;

    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());

    fixture.efb.numbers[kFuelTargetDataref] = kPlannedFuelKg;

    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());

    fixture.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;

    QVERIFY(fixture.aircraft.IsFlightPlanLoaded());

    fixture.efb.numbers[kSimbriefImported] = 0.0;

    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());
}

void FenixA32xTest::loadingStartArmsThirdPartyRefueling()
{
    FenixFixture fixture;

    fixture.aircraft.OnLoadingStarted();

    QCOMPARE(fixture.gateway.Written(kThirdPartyRefuel), 1.0);
}

void FenixA32xTest::thirdPartyRefuelingDisarmsWhenGsxCompletes()
{
    FenixFixture fixture;

    fixture.aircraft.OnLoadingStarted();
    fixture.gateway.lvars[gsx::lvars::kRefuelingState] = kGsxStateActive;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.gateway.Written(kThirdPartyRefuel), 1.0);

    fixture.gateway.lvars[gsx::lvars::kRefuelingState] = kGsxStateCompleted;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.gateway.Written(kThirdPartyRefuel), 0.0);
}

void FenixA32xTest::fuelSetterWritesEfbOncePerValue()
{
    FenixFixture fixture;

    fixture.aircraft.SetCurrentFuelKg(5000.0);
    fixture.aircraft.SetCurrentFuelKg(5000.0);

    QCOMPARE(fixture.efb.setFloatCalls, 1);
    QCOMPARE(fixture.efb.WrittenFloat(kFuelDataref), 5000.0);

    fixture.aircraft.SetCurrentFuelKg(5028.0);

    QCOMPARE(fixture.efb.setFloatCalls, 2);
    QCOMPARE(fixture.efb.WrittenFloat(kFuelDataref), 5028.0);
}

void FenixA32xTest::fuelSetterSilentWhileEfbUnavailable()
{
    FenixFixture fixture;

    fixture.efb.available = false;

    fixture.aircraft.SetCurrentFuelKg(5000.0);

    QCOMPARE(fixture.efb.setFloatCalls, 0);

    fixture.efb.available = true;
    fixture.aircraft.SetCurrentFuelKg(5000.0);

    QCOMPARE(fixture.efb.WrittenFloat(kFuelDataref), 5000.0);
}

void FenixA32xTest::zfwSetterBoardsSeatsAndCargoProgressively()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.SetCurrentZfwKg(50000.0);

    const std::string seatString = fixture.efb.WrittenString(kSeatOccupationString);
    constexpr double cargoKg = kPlannedCargoKg / 2.0;
    constexpr double fwdCargoKg = cargoKg * 0.4237;

    QCOMPARE(CountSeats(seatString), kSeatCapacity);
    QCOMPARE(CountOccupiedSeats(seatString), kPlannedPassengers / 2);
    QVERIFY(std::abs(fixture.efb.WrittenFloat(kFwdCargo) - fwdCargoKg) < 0.01);
    QVERIFY(std::abs(fixture.efb.WrittenFloat(kAftCargo) - fwdCargoKg) < 0.01);
    QVERIFY(std::abs(fixture.efb.WrittenFloat(kBulkCargo) - (cargoKg - 2.0 * fwdCargoKg)) < 0.01);
}

void FenixA32xTest::zfwSetterSnapsToPlannedOnCompletion()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.SetCurrentZfwKg(kPlannedZfwKg);

    const double totalCargoKg = fixture.efb.WrittenFloat(kFwdCargo)
        + fixture.efb.WrittenFloat(kAftCargo)
        + fixture.efb.WrittenFloat(kBulkCargo);

    QCOMPARE(CountOccupiedSeats(fixture.efb.WrittenString(kSeatOccupationString)), kPlannedPassengers);
    QVERIFY(std::abs(totalCargoKg - kPlannedCargoKg) < 0.01);
}

void FenixA32xTest::zfwSetterDrainsToEmptyOnDeboard()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.SetCurrentZfwKg(kPlannedZfwKg);
    fixture.aircraft.SetCurrentZfwKg(kEmptyWeightKg);

    QCOMPARE(CountOccupiedSeats(fixture.efb.WrittenString(kSeatOccupationString)), 0);
    QCOMPARE(fixture.efb.WrittenFloat(kFwdCargo), 0.0);
    QCOMPARE(fixture.efb.WrittenFloat(kAftCargo), 0.0);
    QCOMPARE(fixture.efb.WrittenFloat(kBulkCargo), 0.0);
}

void FenixA32xTest::zfwSetterIgnoresRepeatedValues()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.SetCurrentZfwKg(50000.0);
    const int floatWrites = fixture.efb.setFloatCalls;
    const int stringWrites = fixture.efb.setStringCalls;
    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.setFloatCalls, floatWrites);
    QCOMPARE(fixture.efb.setStringCalls, stringWrites);
}

void FenixA32xTest::zfwSetterSilentWhileEfbUnavailable()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();
    fixture.efb.available = false;

    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.setFloatCalls, 0);
    QCOMPARE(fixture.efb.setStringCalls, 0);

    fixture.efb.available = true;
    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(CountOccupiedSeats(fixture.efb.WrittenString(kSeatOccupationString)), kPlannedPassengers / 2);
}

void FenixA32xTest::zfwSetterWaitsForEmptyWeightData()
{
    FenixFixture fixture;

    fixture.SeedBookedSeats();
    fixture.efb.numbers[kCargoTargetDataref] = kPlannedCargoKg;

    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.setFloatCalls, 0);
    QCOMPARE(fixture.efb.setStringCalls, 0);
}

void FenixA32xTest::zfwSetterSkipsWithoutPlannedZfw()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;

    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.setFloatCalls, 0);
    QCOMPARE(fixture.efb.setStringCalls, 0);
}

void FenixA32xTest::seatWritesWaitForBookedSeats()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();
    fixture.efb.boolArrays.clear();

    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.setStringCalls, 0);
    QVERIFY(fixture.efb.setFloatCalls > 0);
}

void FenixA32xTest::preliminaryLoadsheetRequestedWhenLoadingStarts()
{
    FenixFixture fixture;

    fixture.aircraft.OnLoadingStarted();

    QCOMPARE(fixture.efb.loadsheetRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.efb.loadsheetRequests.front()), QString("Preliminary"));
}

void FenixA32xTest::preliminaryLoadsheetSkippedWhileEfbUnavailable()
{
    FenixFixture fixture;

    fixture.efb.available = false;

    fixture.aircraft.OnLoadingStarted();

    QVERIFY(fixture.efb.loadsheetRequests.empty());
}

void FenixA32xTest::finalLoadsheetRequestedOnceAtPlannedZfw()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.OnLoadingStarted();
    fixture.aircraft.SetCurrentZfwKg(50000.0);

    QCOMPARE(fixture.efb.loadsheetRequests.size(), static_cast<std::size_t>(1));

    fixture.aircraft.SetCurrentZfwKg(kPlannedZfwKg);
    fixture.aircraft.SetCurrentZfwKg(kPlannedZfwKg - 10.0);

    QCOMPARE(fixture.efb.loadsheetRequests.size(), static_cast<std::size_t>(2));
    QCOMPARE(QString::fromStdString(fixture.efb.loadsheetRequests.back()), QString("Final"));
}

void FenixA32xTest::finalLoadsheetWaitsForLoadingStart()
{
    FenixFixture fixture;

    fixture.SeedPlannedLoad();

    fixture.aircraft.SetCurrentZfwKg(kPlannedZfwKg);

    QVERIFY(fixture.efb.loadsheetRequests.empty());
}

void FenixA32xTest::cargoDoorsFollowBaggageLoaders()
{
    FenixFixture fixture;

    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 6.0;
    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 9.0;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdCargoDoor), 1);
    QCOMPARE(fixture.efb.WrittenBool(kAftCargoDoor), 1);

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 1.0;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdCargoDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kAftCargoDoor), 1);
}

void FenixA32xTest::paxDoorOpensForJetwayOrFrontStairs()
{
    FenixFixture fixture;

    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kJetway] = 5.0;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), 1);

    fixture.gateway.lvars[gsx::lvars::kJetway] = 2.0;
    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 3.0;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), 1);

    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 4.0;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), 0);
}

void FenixA32xTest::midPaxDoorDrivenOnlyOnA321()
{
    FenixFixture a320{FenixVariant::A320};

    a320.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    a320.gateway.lvars[gsx::lvars::kPassengerStairsMiddleState] = 3.0;

    a320.aircraft.OnTick();

    QCOMPARE(a320.efb.WrittenBool(kMidPaxDoor), -1);

    FenixFixture a321{FenixVariant::A321};
    a321.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    a321.gateway.lvars[gsx::lvars::kPassengerStairsMiddleState] = 3.0;

    a321.aircraft.OnTick();

    QCOMPARE(a321.efb.WrittenBool(kMidPaxDoor), 1);
}

void FenixA32xTest::cateringDoorsFollowCateringVehicles()
{
    FenixFixture fixture;

    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kCateringFrontState] = 6.0;
    fixture.gateway.lvars[gsx::lvars::kCateringRearState] = 7.0;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdCateringDoor), 1);
    QCOMPARE(fixture.efb.WrittenBool(kAftCateringDoor), 1);

    fixture.gateway.lvars[gsx::lvars::kCateringRearState] = 8.0;
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kAftCateringDoor), 0);
}

void FenixA32xTest::doorsUntouchedWithoutGsx()
{
    FenixFixture fixture;

    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 3.0;
    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 6.0;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), -1);
    QCOMPARE(fixture.efb.WrittenBool(kFwdCargoDoor), -1);
}

void FenixA32xTest::doorsUntouchedWhileEfbUnavailable()
{
    FenixFixture fixture;

    fixture.efb.available = false;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 3.0;

    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.setBoolCalls, 0);
}

void FenixA32xTest::closeAllDoorsForcesEveryDoorClosed()
{
    FenixFixture fixture;

    fixture.aircraft.CloseAllDoors();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kMidPaxDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kAftPaxDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kFwdCateringDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kAftCateringDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kFwdCargoDoor), 0);
    QCOMPARE(fixture.efb.WrittenBool(kAftCargoDoor), 0);
}

void FenixA32xTest::stairsReopenDoorAfterCloseAllDoors()
{
    FenixFixture fixture;

    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 3.0;

    fixture.aircraft.OnTick();
    fixture.aircraft.CloseAllDoors();
    fixture.aircraft.OnTick();

    QCOMPARE(fixture.efb.WrittenBool(kFwdPaxDoor), 1);
}

void FenixA32xTest::groundPowerWritesEfbDataref()
{
    FenixFixture fixture;

    fixture.aircraft.SetGroundPower(true);

    QCOMPARE(fixture.efb.WrittenBool(kGroundPowerDataref), 1);

    fixture.aircraft.SetGroundPower(false);

    QCOMPARE(fixture.efb.WrittenBool(kGroundPowerDataref), 0);
}

void FenixA32xTest::groundPowerStatusFollowsGpuAndExtPower()
{
    FenixFixture fixture;

    QVERIFY(fixture.aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);

    fixture.gateway.lvars[kGpuPlaced] = 0.0;

    QVERIFY(fixture.aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    fixture.gateway.lvars[kGpuPlaced] = 1.0;

    QVERIFY(fixture.aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);

    fixture.gateway.lvars[kGpuPlaced] = 0.0;
    fixture.gateway.lvars[kExtPowerOnBus] = 1.0;

    QVERIFY(fixture.aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);
}

void FenixA32xTest::chocksWriteEfbDataref()
{
    FenixFixture fixture;

    fixture.aircraft.SetChocks(true);

    QCOMPARE(fixture.efb.WrittenBool(kChocksDataref), 1);

    fixture.aircraft.SetChocks(false);

    QCOMPARE(fixture.efb.WrittenBool(kChocksDataref), 0);
}

void FenixA32xTest::smartSwitchFiresOnceAndResetsTheSwitch()
{
    FenixFixture fixture;

    fixture.gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(fixture.aircraft.ConsumeSmartSwitch());
    QCOMPARE(fixture.gateway.Written(kSmartSwitch), 1.0);
    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());

    fixture.gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(fixture.aircraft.ConsumeSmartSwitch());
}

void FenixA32xTest::smartSwitchIgnoresRadioPosition()
{
    FenixFixture fixture;

    fixture.gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());
    QCOMPARE(fixture.gateway.setLVarCalls, 0);
}

void FenixA32xTest::smartSwitchStaysQuietUntilDataArrives()
{
    FenixFixture fixture;

    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());
    QCOMPARE(fixture.gateway.setLVarCalls, 0);
}

void FenixA32xTest::powerRequiresBatteryPlusExternalOrApu()
{
    FenixFixture fixture;

    QVERIFY(!fixture.aircraft.IsPowered());

    fixture.gateway.lvars[kBattery1] = 1.0;
    fixture.gateway.lvars[kDcEssBus] = 1.0;

    QVERIFY(!fixture.aircraft.IsPowered());

    fixture.gateway.lvars[kExtPowerOnBus] = 1.0;

    QVERIFY(fixture.aircraft.IsPowered());

    fixture.gateway.lvars[kExtPowerOnBus] = 0.0;
    fixture.gateway.lvars[kApuRunning] = 1.0;

    QVERIFY(fixture.aircraft.IsPowered());

    fixture.gateway.lvars[kBattery1] = 0.0;
    fixture.gateway.lvars[kBattery2] = 1.0;

    QVERIFY(fixture.aircraft.IsPowered());
}

void FenixA32xTest::engineAssumedRunningUntilDataArrives()
{
    const FenixFixture fixture;

    QVERIFY(fixture.aircraft.IsEngineRunning());
}

void FenixA32xTest::engineRunningDetectsEitherEngine()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEng1Combustion] = 0.0;
    fixture.gateway.avars[kSimEng2Combustion] = 0.0;

    QVERIFY(!fixture.aircraft.IsEngineRunning());

    fixture.gateway.avars[kSimEng2Combustion] = 1.0;

    QVERIFY(fixture.aircraft.IsEngineRunning());
}

void FenixA32xTest::parkingBrakeRequiresBothSources()
{
    FenixFixture fixture;

    fixture.gateway.lvars[kParkingBrakeLvar] = 1.0;

    QVERIFY(!fixture.aircraft.IsParkingBrakeSet());

    fixture.gateway.avars[kSimParkingBrake] = 1.0;

    QVERIFY(fixture.aircraft.IsParkingBrakeSet());
}

void FenixA32xTest::readyToPushFollowsPowerBeaconAndEngines()
{
    FenixFixture fixture;

    fixture.gateway.lvars[kDcEssBus] = 1.0;
    fixture.gateway.lvars[kBattery1] = 1.0;
    fixture.gateway.lvars[kApuRunning] = 1.0;
    fixture.gateway.avars[kSimEng1Combustion] = 0.0;
    fixture.gateway.avars[kSimEng2Combustion] = 0.0;
    fixture.gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(fixture.aircraft.IsReadyToPush());

    fixture.gateway.avars[kSimBeaconLight] = 0.0;

    QVERIFY(!fixture.aircraft.IsReadyToPush());
}

void FenixA32xTest::readyToDeboardFollowsSafetyState()
{
    FenixFixture fixture;

    fixture.gateway.avars[kSimEng1Combustion] = 0.0;
    fixture.gateway.avars[kSimEng2Combustion] = 0.0;
    fixture.gateway.avars[kSimBeaconLight] = 0.0;

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());

    fixture.gateway.lvars[kChocks] = 1.0;

    QVERIFY(fixture.aircraft.IsReadyToDeboard());

    fixture.gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());
}

QTEST_APPLESS_MAIN(FenixA32xTest)

#include "tst_fenix_a32x.moc"
