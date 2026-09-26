#include <QtTest/QTest>

#include "../src/domain/model/AutomationSettings.h"
#include "../src/application/model/EffectiveSettings.h"

namespace
{
    enum class ProfileState
    {
        Absent,
        MirrorsGlobal,
        Custom
    };

    constexpr auto kProfileId = "fenix-a320";
    constexpr double kRecommendedKgs = 17.0;
    constexpr double kGlobalManualKgs = 25.0;
    constexpr double kProfileManualKgs = 12.5;
}

class AutomationSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    static void defaultsUseDefaultFuelRate();
    static void positiveFuelRateUsedAsIs();
    static void zeroOrNegativeFallsBackToDefault();
    static void resolvesGlobalsWhenProfileMissing();
    static void resolvesGlobalsWhenProfileUsesGlobal();
    static void customProfileOverridesAutomationFields();
    static void customProfileKeepsPilotIdAndAutoFlags();
    static void aircraftCarryingItsOwnStairsOverridesTheSetting();
    static void resolvesTheFuelRateByTheModes_data();
    static void resolvesTheFuelRateByTheModes();
    static void aGlobalModeStoredOnTheGlobalRateReadsAsRecommended();
    static void earlyBoardingFollowsTheGlobalUnlessTheProfileIsCustom();
};

void AutomationSettingsTest::earlyBoardingFollowsTheGlobalUnlessTheProfileIsCustom()
{
    AppSettings settings;

    QVERIFY(!ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs).callBoardingEarly);

    settings.callBoardingEarly = true;

    QVERIFY(ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs).callBoardingEarly);

    AircraftProfile profile;
    profile.useGlobal = true;
    profile.callBoardingEarly = false;
    settings.profiles[kProfileId] = profile;

    QVERIFY(ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs).callBoardingEarly);

    profile.useGlobal = false;
    settings.profiles[kProfileId] = profile;

    QVERIFY(!ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs).callBoardingEarly);

    settings.callBoardingEarly = false;
    profile.callBoardingEarly = true;
    settings.profiles[kProfileId] = profile;

    QVERIFY(ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs).callBoardingEarly);
}

void AutomationSettingsTest::defaultsUseDefaultFuelRate()
{
    constexpr AutomationSettings settings;

    QCOMPARE(settings.fuelRateKgs, AutomationSettings::kDefaultFuelRateKgs);
    QCOMPARE(settings.EffectiveFuelRateKgs(), AutomationSettings::kDefaultFuelRateKgs);
}

void AutomationSettingsTest::positiveFuelRateUsedAsIs()
{
    AutomationSettings settings;

    settings.fuelRateKgs = 175.5;

    QCOMPARE(settings.EffectiveFuelRateKgs(), 175.5);
}

void AutomationSettingsTest::zeroOrNegativeFallsBackToDefault()
{
    AutomationSettings settings;

    settings.fuelRateKgs = 0.0;

    QCOMPARE(settings.EffectiveFuelRateKgs(), AutomationSettings::kDefaultFuelRateKgs);

    settings.fuelRateKgs = -50.0;

    QCOMPARE(settings.EffectiveFuelRateKgs(), AutomationSettings::kDefaultFuelRateKgs);
}

void AutomationSettingsTest::resolvesGlobalsWhenProfileMissing()
{
    AppSettings settings;
    settings.fuelRateMode = FuelRateMode::Manual;
    settings.fuelRateKgs = 25.0;
    settings.callGpu = true;
    settings.callGpuOnArrival = true;

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "unknown-id", false, 0.0);

    QCOMPARE(resolved.fuelRateKgs, 25.0);
    QVERIFY(resolved.callGpu);
    QVERIFY(resolved.callGpuOnArrival);
}

void AutomationSettingsTest::resolvesGlobalsWhenProfileUsesGlobal()
{
    AppSettings settings;
    settings.skipReposition = true;
    AircraftProfile profile;
    profile.useGlobal = true;
    profile.skipReposition = false;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340", false, 0.0);

    QVERIFY(resolved.skipReposition);
}

void AutomationSettingsTest::customProfileOverridesAutomationFields()
{
    AppSettings settings;
    settings.fuelRateKgs = 60.0;
    settings.callCatering = false;
    settings.callWater = false;
    AircraftProfile profile;
    profile.useGlobal = false;
    profile.fuelRateMode = FuelRateMode::Manual;
    profile.fuelRateKgs = 12.5;
    profile.skipReposition = true;
    profile.callGpu = true;
    profile.callGpuOnArrival = true;
    profile.callCatering = true;
    profile.callLavatory = true;
    profile.callWater = true;
    profile.callCleaning = true;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340", false, 0.0);

    QCOMPARE(resolved.fuelRateKgs, 12.5);
    QVERIFY(resolved.skipReposition);
    QVERIFY(resolved.callGpu);
    QVERIFY(resolved.callGpuOnArrival);
    QVERIFY(resolved.callCatering);
    QVERIFY(resolved.callLavatory);
    QVERIFY(resolved.callWater);
    QVERIFY(resolved.callCleaning);
}

void AutomationSettingsTest::customProfileKeepsPilotIdAndAutoFlags()
{
    AppSettings settings;
    settings.simbriefPilotId = 42;
    settings.autoSelectGsxChoice = false;
    settings.autoDeice = true;
    settings.useAircraftStairs = true;
    settings.crewBoarding = 1;
    settings.crewDeboarding = 2;
    settings.autoStartFlow = true;
    settings.autoStartLoading = false;
    AircraftProfile profile;
    profile.useGlobal = false;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340", false, 0.0);

    QCOMPARE(resolved.simbriefPilotId, 42);
    QVERIFY(!resolved.autoSelectGsxChoice);
    QVERIFY(resolved.autoDeice);
    QVERIFY(resolved.useAircraftStairs);
    QCOMPARE(resolved.crewBoarding, CrewChoice::Crew);
    QCOMPARE(resolved.crewDeboarding, CrewChoice::Pilots);
    QVERIFY(resolved.autoStartFlow);
    QVERIFY(!resolved.autoStartLoading);
}

void AutomationSettingsTest::aircraftCarryingItsOwnStairsOverridesTheSetting()
{
    AppSettings settings;
    settings.useAircraftStairs = false;

    QVERIFY(!ResolveAutomationSettings(settings, "justflight-rj85", false, 12.0).useAircraftStairs);
    QVERIFY(ResolveAutomationSettings(settings, "justflight-rj85", true, 12.0).useAircraftStairs);
}

void AutomationSettingsTest::resolvesTheFuelRateByTheModes_data()
{
    QTest::addColumn<FuelRateMode>("globalMode");
    QTest::addColumn<double>("globalKgs");
    QTest::addColumn<ProfileState>("profileState");
    QTest::addColumn<FuelRateMode>("profileMode");
    QTest::addColumn<double>("profileKgs");
    QTest::addColumn<double>("recommendedKgs");
    QTest::addColumn<double>("expected");

    QTest::newRow("no profile, global recommended")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::Absent
        << FuelRateMode::Manual << kProfileManualKgs << kRecommendedKgs << kRecommendedKgs;
    QTest::newRow("no profile, global manual")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Absent
        << FuelRateMode::Recommended << kProfileManualKgs << kRecommendedKgs << kGlobalManualKgs;
    QTest::newRow("no profile, global recommended of zero")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::Absent
        << FuelRateMode::Manual << kProfileManualKgs << 0.0 << AutomationSettings::kDefaultFuelRateKgs;
    QTest::newRow("no profile, global manual invalid")
        << FuelRateMode::Manual << 0.0 << ProfileState::Absent
        << FuelRateMode::Recommended << kProfileManualKgs << kRecommendedKgs << AutomationSettings::kDefaultFuelRateKgs;

    QTest::newRow("mirrored profile, global recommended")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::MirrorsGlobal
        << FuelRateMode::Manual << kProfileManualKgs << kRecommendedKgs << kRecommendedKgs;
    QTest::newRow("mirrored profile, global manual")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::MirrorsGlobal
        << FuelRateMode::Recommended << kProfileManualKgs << kRecommendedKgs << kGlobalManualKgs;
    QTest::newRow("mirrored profile, global recommended of zero")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::MirrorsGlobal
        << FuelRateMode::Manual << kProfileManualKgs << 0.0 << AutomationSettings::kDefaultFuelRateKgs;

    QTest::newRow("custom profile global, global recommended")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Global << kProfileManualKgs << kRecommendedKgs << kRecommendedKgs;
    QTest::newRow("custom profile global, global manual")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Global << kProfileManualKgs << kRecommendedKgs << kGlobalManualKgs;
    QTest::newRow("custom profile global, global manual invalid")
        << FuelRateMode::Manual << -3.0 << ProfileState::Custom
        << FuelRateMode::Global << kProfileManualKgs << kRecommendedKgs << AutomationSettings::kDefaultFuelRateKgs;

    QTest::newRow("custom profile recommended, global recommended")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Recommended << kProfileManualKgs << kRecommendedKgs << kRecommendedKgs;
    QTest::newRow("custom profile recommended, global manual")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Recommended << kProfileManualKgs << kRecommendedKgs << kRecommendedKgs;
    QTest::newRow("custom profile recommended of zero")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Recommended << kProfileManualKgs << 0.0 << AutomationSettings::kDefaultFuelRateKgs;

    QTest::newRow("custom profile manual, global recommended")
        << FuelRateMode::Recommended << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Manual << kProfileManualKgs << kRecommendedKgs << kProfileManualKgs;
    QTest::newRow("custom profile manual, global manual")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Manual << kProfileManualKgs << kRecommendedKgs << kProfileManualKgs;
    QTest::newRow("custom profile manual invalid")
        << FuelRateMode::Manual << kGlobalManualKgs << ProfileState::Custom
        << FuelRateMode::Manual << 0.0 << kRecommendedKgs << AutomationSettings::kDefaultFuelRateKgs;
}

void AutomationSettingsTest::resolvesTheFuelRateByTheModes()
{
    QFETCH(const FuelRateMode, globalMode);
    QFETCH(const double, globalKgs);
    QFETCH(const ProfileState, profileState);
    QFETCH(const FuelRateMode, profileMode);
    QFETCH(const double, profileKgs);
    QFETCH(const double, recommendedKgs);
    QFETCH(const double, expected);

    AppSettings settings;
    settings.fuelRateMode = globalMode;
    settings.fuelRateKgs = globalKgs;
    if (profileState != ProfileState::Absent)
    {
        AircraftProfile profile;
        profile.useGlobal = profileState == ProfileState::MirrorsGlobal;
        profile.fuelRateMode = profileMode;
        profile.fuelRateKgs = profileKgs;
        settings.profiles.emplace(kProfileId, profile);
    }

    const AutomationSettings resolved = ResolveAutomationSettings(settings, kProfileId, false, recommendedKgs);

    QCOMPARE(resolved.fuelRateKgs, expected);
}

void AutomationSettingsTest::aGlobalModeStoredOnTheGlobalRateReadsAsRecommended()
{
    AppSettings settings;
    settings.fuelRateMode = FuelRateMode::Global;
    settings.fuelRateKgs = kGlobalManualKgs;

    const AutomationSettings resolved = ResolveAutomationSettings(settings, kProfileId, false, kRecommendedKgs);

    QCOMPARE(resolved.fuelRateKgs, kRecommendedKgs);
}

QTEST_APPLESS_MAIN(AutomationSettingsTest)

#include "tst_automation_settings.moc"
