#include <QtTest/QTest>

#include "../src/domain/model/AutomationSettings.h"

class AutomationSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    static void defaultsUseDefaultFuelRate();
    static void positiveFuelRateUsedAsIs();
    static void zeroOrNegativeFallsBackToDefault();
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

QTEST_APPLESS_MAIN(AutomationSettingsTest)

#include "tst_automation_settings.moc"
