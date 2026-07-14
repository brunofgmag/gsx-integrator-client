#include <QtTest/QTest>

#include "../src/domain/model/AutomationSettings.h"
#include "../src/application/model/EffectiveSettings.h"

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
};

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
    settings.fuelRateKgs = 25.0;
    settings.callGpu = true;

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "unknown-id");

    QCOMPARE(resolved.fuelRateKgs, 25.0);
    QVERIFY(resolved.callGpu);
}

void AutomationSettingsTest::resolvesGlobalsWhenProfileUsesGlobal()
{
    AppSettings settings;
    settings.skipReposition = true;
    AircraftProfile profile;
    profile.useGlobal = true;
    profile.skipReposition = false;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340");

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
    profile.fuelRateKgs = 12.5;
    profile.skipReposition = true;
    profile.callGpu = true;
    profile.callCatering = true;
    profile.callLavatory = true;
    profile.callWater = true;
    profile.callCleaning = true;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340");

    QCOMPARE(resolved.fuelRateKgs, 12.5);
    QVERIFY(resolved.skipReposition);
    QVERIFY(resolved.callGpu);
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
    settings.autoStartFlow = true;
    settings.autoStartLoading = false;
    AircraftProfile profile;
    profile.useGlobal = false;
    settings.profiles.emplace("toliss-a340", profile);

    const AutomationSettings resolved = ResolveAutomationSettings(settings, "toliss-a340");

    QCOMPARE(resolved.simbriefPilotId, 42);
    QVERIFY(!resolved.autoSelectGsxChoice);
    QVERIFY(resolved.autoStartFlow);
    QVERIFY(!resolved.autoStartLoading);
}

QTEST_APPLESS_MAIN(AutomationSettingsTest)

#include "tst_automation_settings.moc"
