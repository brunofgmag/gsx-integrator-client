#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/settings/QSettingsRepository.h"

class QSettingsRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() const;
    static void init();

    void emptyStoreYieldsLoadDefaults() const;
    void saveLoadRoundTrip();
    void useGlobalProfilesAreNotPersisted();
    void themeModeLegacyDarkFallsBackToDark() const;
    void themeModeLegacyLightFallsBackToLight() const;
    void explicitThemeModeWinsOverLegacy() const;
    void saveReplacesExistingProfiles();
    void crewDeboardingInheritsTheBoardingChoiceWhenAbsent() const;
    void explicitCrewDeboardingWinsOverTheBoardingChoice() const;
    void absentPanelKeysYieldTheOnPushbackDefault() const;
    void theLegacyOpenOnRequestsOnBecomesAllRequests() const;
    void theLegacyOpenOnRequestsOffBecomesNever() const;
    void anExplicitPanelModeWinsOverTheLegacyKey() const;
    void theRetiredKeepClosedBecomesOnPushback() const;
    void theRetiredAllRequestsSurvivesTheRescale() const;
    void aRescaledPanelModeIsNeverMigratedTwice() const;
    void savingThePanelModeLeavesTheLegacyKeyIntact();
    void anAbsentGlobalFuelRateModeReadsAsRecommendedAndKeepsTheRateAsManual() const;
    void anAbsentProfileFuelRateModeReadsAsRecommendedAndKeepsTheRateAsManual() const;
    void storedFuelRateModesAreReadBack() const;
    void savedFuelRateModesAreWrittenAndReread();
    void anAbsentEarlyBoardingKeyReadsAsOff() const;

private:
    QTemporaryDir tempDir_;
    QSettingsRepository repository_;
};

void QSettingsRepositoryTest::initTestCase() const
{
    QVERIFY(tempDir_.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("GsxIntegratorTests"));
    QCoreApplication::setApplicationName(QStringLiteral("QSettingsRepositoryTest"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir_.path());
}

void QSettingsRepositoryTest::init()
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

void QSettingsRepositoryTest::emptyStoreYieldsLoadDefaults() const
{
    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.simbriefPilotId, 0);
    QCOMPARE(loaded.fuelRateMode, FuelRateMode::Recommended);
    QCOMPARE(loaded.fuelRateKgs, AutomationSettings::kDefaultFuelRateKgs);
    QCOMPARE(loaded.autoSelectGsxChoice, true);
    QCOMPARE(loaded.autoDeice, false);
    QCOMPARE(loaded.useAircraftStairs, false);
    QCOMPARE(loaded.crewBoarding, 3);
    QCOMPARE(loaded.crewDeboarding, 3);
    QCOMPARE(loaded.autoStartFlow, false);
    QCOMPARE(loaded.autoStartLoading, true);
    QCOMPARE(loaded.skipReposition, false);
    QCOMPARE(loaded.callGpu, false);
    QCOMPARE(loaded.callGpuOnArrival, false);
    QCOMPARE(loaded.callBoardingEarly, false);
    QCOMPARE(loaded.callCatering, false);
    QCOMPARE(loaded.callLavatory, false);
    QCOMPARE(loaded.callWater, false);
    QCOMPARE(loaded.callCleaning, false);
    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::OnPushback));
    QCOMPARE(loaded.themeMode, 2);
    QCOMPARE(loaded.language, std::string("system"));
    QCOMPARE(loaded.renderer, std::string("software"));
    QCOMPARE(loaded.updateMode, 1);
    QCOMPARE(loaded.closeToTray, false);
    QCOMPARE(loaded.minimizeToTray, true);
    QCOMPARE(loaded.trayTipShown, false);
    QCOMPARE(loaded.streamerMode, false);
    QCOMPARE(loaded.loggingEnabled, false);
    QCOMPARE(loaded.commbusManaged, true);
    QVERIFY(loaded.profiles.empty());
}

void QSettingsRepositoryTest::absentPanelKeysYieldTheOnPushbackDefault() const
{
    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::OnPushback));
}

void QSettingsRepositoryTest::theLegacyOpenOnRequestsOnBecomesAllRequests() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/openGsxOnRequests"), true);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::AllRequests));
}

void QSettingsRepositoryTest::theLegacyOpenOnRequestsOffBecomesNever() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/openGsxOnRequests"), false);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::Never));
}

void QSettingsRepositoryTest::anExplicitPanelModeWinsOverTheLegacyKey() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/openGsxOnRequests"), true);
    settings.setValue(QStringLiteral("gsx/panelMode"), static_cast<int>(GsxPanelMode::Never));
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::Never));
}

void QSettingsRepositoryTest::theRetiredKeepClosedBecomesOnPushback() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/panelMode"), 1);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::OnPushback));
}

void QSettingsRepositoryTest::theRetiredAllRequestsSurvivesTheRescale() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/panelMode"), 3);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::AllRequests));
}

void QSettingsRepositoryTest::aRescaledPanelModeIsNeverMigratedTwice() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/panelMode"), static_cast<int>(GsxPanelMode::AllRequests));
    settings.setValue(QStringLiteral("gsx/panelModeScale"), 2);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.gsxPanelMode, static_cast<int>(GsxPanelMode::AllRequests));
}

void QSettingsRepositoryTest::savingThePanelModeLeavesTheLegacyKeyIntact()
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/openGsxOnRequests"), false);
    settings.sync();

    AppSettings values;
    values.gsxPanelMode = static_cast<int>(GsxPanelMode::AllRequests);

    QVERIFY(repository_.Save(values));

    QSettings reread;

    QCOMPARE(reread.value(QStringLiteral("gsx/openGsxOnRequests")).toBool(), false);
    QCOMPARE(reread.value(QStringLiteral("gsx/panelMode")).toInt(),
             static_cast<int>(GsxPanelMode::AllRequests));
}

void QSettingsRepositoryTest::anAbsentGlobalFuelRateModeReadsAsRecommendedAndKeepsTheRateAsManual() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("fuel/rateKgs"), 42.5);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.fuelRateMode, FuelRateMode::Recommended);
    QCOMPARE(loaded.fuelRateKgs, 42.5);
}

void QSettingsRepositoryTest::anAbsentProfileFuelRateModeReadsAsRecommendedAndKeepsTheRateAsManual() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("profiles/fenix-a320/useGlobal"), false);
    settings.setValue(QStringLiteral("profiles/fenix-a320/fuelRateKgs"), 33.0);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    const auto it = loaded.profiles.find("fenix-a320");
    QVERIFY(it != loaded.profiles.end());
    QCOMPARE(it->second.fuelRateMode, FuelRateMode::Recommended);
    QCOMPARE(it->second.fuelRateKgs, 33.0);
}

void QSettingsRepositoryTest::storedFuelRateModesAreReadBack() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("fuel/rateMode"), static_cast<int>(FuelRateMode::Manual));
    settings.setValue(QStringLiteral("profiles/fenix-a320/useGlobal"), false);
    settings.setValue(QStringLiteral("profiles/fenix-a320/fuelRateMode"), static_cast<int>(FuelRateMode::Manual));
    settings.setValue(QStringLiteral("profiles/pmdg-777f/useGlobal"), false);
    settings.setValue(QStringLiteral("profiles/pmdg-777f/fuelRateMode"), static_cast<int>(FuelRateMode::Global));
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.fuelRateMode, FuelRateMode::Manual);
    QCOMPARE(loaded.profiles.at("fenix-a320").fuelRateMode, FuelRateMode::Manual);
    QCOMPARE(loaded.profiles.at("pmdg-777f").fuelRateMode, FuelRateMode::Global);
}

void QSettingsRepositoryTest::savedFuelRateModesAreWrittenAndReread()
{
    AppSettings values;
    values.fuelRateMode = FuelRateMode::Manual;
    values.fuelRateKgs = 18.0;
    AircraftProfile recommended;
    recommended.useGlobal = false;
    recommended.fuelRateMode = FuelRateMode::Recommended;
    recommended.fuelRateKgs = 11.0;
    values.profiles.emplace("fenix-a320", recommended);
    AircraftProfile manual;
    manual.useGlobal = false;
    manual.fuelRateMode = FuelRateMode::Manual;
    manual.fuelRateKgs = 44.0;
    values.profiles.emplace("pmdg-777f", manual);

    QVERIFY(repository_.Save(values));

    const QSettings reread;
    QCOMPARE(reread.value(QStringLiteral("fuel/rateMode")).toInt(), static_cast<int>(FuelRateMode::Manual));
    QCOMPARE(reread.value(QStringLiteral("profiles/fenix-a320/fuelRateMode")).toInt(),
             static_cast<int>(FuelRateMode::Recommended));
    QCOMPARE(reread.value(QStringLiteral("profiles/pmdg-777f/fuelRateMode")).toInt(),
             static_cast<int>(FuelRateMode::Manual));

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.fuelRateMode, FuelRateMode::Manual);
    QCOMPARE(loaded.fuelRateKgs, 18.0);
    QCOMPARE(loaded.profiles.at("fenix-a320").fuelRateMode, FuelRateMode::Recommended);
    QCOMPARE(loaded.profiles.at("fenix-a320").fuelRateKgs, 11.0);
    QCOMPARE(loaded.profiles.at("pmdg-777f").fuelRateMode, FuelRateMode::Manual);
    QCOMPARE(loaded.profiles.at("pmdg-777f").fuelRateKgs, 44.0);
}

void QSettingsRepositoryTest::crewDeboardingInheritsTheBoardingChoiceWhenAbsent() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/crewBoarding"), 2);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.crewBoarding, 2);
    QCOMPARE(loaded.crewDeboarding, 2);
}

void QSettingsRepositoryTest::explicitCrewDeboardingWinsOverTheBoardingChoice() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("gsx/crewBoarding"), 2);
    settings.setValue(QStringLiteral("gsx/crewDeboarding"), 0);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.crewBoarding, 2);
    QCOMPARE(loaded.crewDeboarding, 0);
}

void QSettingsRepositoryTest::saveLoadRoundTrip()
{
    AppSettings values;
    values.simbriefPilotId = 12345;
    values.streamerMode = true;
    values.fuelRateMode = FuelRateMode::Manual;
    values.fuelRateKgs = 42.5;
    values.autoSelectGsxChoice = false;
    values.autoDeice = true;
    values.useAircraftStairs = true;
    values.crewBoarding = 1;
    values.crewDeboarding = 2;
    values.autoStartFlow = true;
    values.autoStartLoading = false;
    values.skipReposition = true;
    values.callGpu = true;
    values.callGpuOnArrival = true;
    values.callBoardingEarly = true;
    values.callCatering = true;
    values.callLavatory = true;
    values.callWater = true;
    values.callCleaning = true;
    values.gsxPanelMode = static_cast<int>(GsxPanelMode::AllRequests);
    values.themeMode = 0;
    values.language = "pt_BR";
    values.renderer = "opengl";
    values.updateMode = 2;
    values.closeToTray = true;
    values.minimizeToTray = false;
    values.trayTipShown = true;
    values.loggingEnabled = true;
    values.commbusManaged = false;

    AircraftProfile profile;
    profile.useGlobal = false;
    profile.fuelRateMode = FuelRateMode::Global;
    profile.fuelRateKgs = 33.0;
    profile.skipReposition = true;
    profile.callGpu = true;
    profile.callGpuOnArrival = false;
    profile.callBoardingEarly = true;
    profile.callCatering = true;
    profile.callLavatory = false;
    profile.callWater = true;
    profile.callCleaning = false;
    values.profiles.emplace("a340", profile);

    QVERIFY(repository_.Save(values));

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.simbriefPilotId, values.simbriefPilotId);
    QCOMPARE(loaded.streamerMode, values.streamerMode);
    QCOMPARE(loaded.commbusManaged, values.commbusManaged);
    QCOMPARE(loaded.fuelRateMode, values.fuelRateMode);
    QCOMPARE(loaded.fuelRateKgs, values.fuelRateKgs);
    QCOMPARE(loaded.autoSelectGsxChoice, values.autoSelectGsxChoice);
    QCOMPARE(loaded.autoDeice, values.autoDeice);
    QCOMPARE(loaded.useAircraftStairs, values.useAircraftStairs);
    QCOMPARE(loaded.crewBoarding, values.crewBoarding);
    QCOMPARE(loaded.crewDeboarding, values.crewDeboarding);
    QCOMPARE(loaded.autoStartFlow, values.autoStartFlow);
    QCOMPARE(loaded.autoStartLoading, values.autoStartLoading);
    QCOMPARE(loaded.skipReposition, values.skipReposition);
    QCOMPARE(loaded.callGpu, values.callGpu);
    QCOMPARE(loaded.callGpuOnArrival, values.callGpuOnArrival);
    QCOMPARE(loaded.callBoardingEarly, values.callBoardingEarly);
    QCOMPARE(loaded.callCatering, values.callCatering);
    QCOMPARE(loaded.callLavatory, values.callLavatory);
    QCOMPARE(loaded.callWater, values.callWater);
    QCOMPARE(loaded.callCleaning, values.callCleaning);
    QCOMPARE(loaded.gsxPanelMode, values.gsxPanelMode);
    QCOMPARE(loaded.themeMode, values.themeMode);
    QCOMPARE(loaded.language, values.language);
    QCOMPARE(loaded.renderer, values.renderer);
    QCOMPARE(loaded.updateMode, values.updateMode);
    QCOMPARE(loaded.closeToTray, values.closeToTray);
    QCOMPARE(loaded.minimizeToTray, values.minimizeToTray);
    QCOMPARE(loaded.trayTipShown, values.trayTipShown);
    QCOMPARE(loaded.loggingEnabled, values.loggingEnabled);

    QCOMPARE(loaded.profiles.size(), std::size_t{1});
    const auto it = loaded.profiles.find("a340");
    QVERIFY(it != loaded.profiles.end());
    QCOMPARE(it->second.useGlobal, profile.useGlobal);
    QCOMPARE(it->second.fuelRateMode, profile.fuelRateMode);
    QCOMPARE(it->second.fuelRateKgs, profile.fuelRateKgs);
    QCOMPARE(it->second.skipReposition, profile.skipReposition);
    QCOMPARE(it->second.callGpu, profile.callGpu);
    QCOMPARE(it->second.callGpuOnArrival, profile.callGpuOnArrival);
    QCOMPARE(it->second.callBoardingEarly, profile.callBoardingEarly);
    QCOMPARE(it->second.callCatering, profile.callCatering);
    QCOMPARE(it->second.callLavatory, profile.callLavatory);
    QCOMPARE(it->second.callWater, profile.callWater);
    QCOMPARE(it->second.callCleaning, profile.callCleaning);
}

void QSettingsRepositoryTest::useGlobalProfilesAreNotPersisted()
{
    AppSettings values;
    AircraftProfile globalProfile;
    globalProfile.useGlobal = true;
    globalProfile.fuelRateKgs = 99.0;
    values.profiles.emplace("md11", globalProfile);

    QVERIFY(repository_.Save(values));

    const AppSettings loaded = repository_.Load();

    QVERIFY(loaded.profiles.empty());
}

void QSettingsRepositoryTest::themeModeLegacyDarkFallsBackToDark() const
{
    QSettings settings;
    settings.setValue("ui/darkTheme", true);
    settings.sync();

    QCOMPARE(repository_.Load().themeMode, 1);
}

void QSettingsRepositoryTest::themeModeLegacyLightFallsBackToLight() const
{
    QSettings settings;
    settings.setValue("ui/darkTheme", false);
    settings.sync();

    QCOMPARE(repository_.Load().themeMode, 2);
}

void QSettingsRepositoryTest::explicitThemeModeWinsOverLegacy() const
{
    QSettings settings;
    settings.setValue("ui/darkTheme", true);
    settings.setValue("ui/themeMode", 0);
    settings.sync();

    QCOMPARE(repository_.Load().themeMode, 0);
}

void QSettingsRepositoryTest::saveReplacesExistingProfiles()
{
    AppSettings first;
    AircraftProfile profileOne;
    profileOne.useGlobal = false;
    profileOne.fuelRateKgs = 11.0;
    first.profiles.emplace("one", profileOne);

    QVERIFY(repository_.Save(first));

    AppSettings second;
    AircraftProfile profileTwo;
    profileTwo.useGlobal = false;
    profileTwo.fuelRateKgs = 22.0;
    second.profiles.emplace("two", profileTwo);

    QVERIFY(repository_.Save(second));

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.profiles.size(), std::size_t{1});
    QVERIFY(loaded.profiles.contains("two"));
}

void QSettingsRepositoryTest::anAbsentEarlyBoardingKeyReadsAsOff() const
{
    QSettings settings;
    settings.setValue("services/callGpu", true);
    settings.setValue("profiles/a340/useGlobal", false);
    settings.setValue("profiles/a340/callGpu", true);
    settings.sync();

    const AppSettings loaded = repository_.Load();

    QVERIFY(!loaded.callBoardingEarly);
    QVERIFY(!loaded.profiles.at("a340").callBoardingEarly);
}

QTEST_GUILESS_MAIN(QSettingsRepositoryTest)

#include "tst_qsettings_repository.moc"
