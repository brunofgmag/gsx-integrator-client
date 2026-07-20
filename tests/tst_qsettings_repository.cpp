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
    QCOMPARE(loaded.fuelRateKgs, AutomationSettings::kDefaultFuelRateKgs);
    QCOMPARE(loaded.autoSelectGsxChoice, true);
    QCOMPARE(loaded.autoDeice, false);
    QCOMPARE(loaded.crewBoarding, 3);
    QCOMPARE(loaded.autoStartFlow, false);
    QCOMPARE(loaded.autoStartLoading, true);
    QCOMPARE(loaded.skipReposition, false);
    QCOMPARE(loaded.callGpu, false);
    QCOMPARE(loaded.callGpuOnArrival, false);
    QCOMPARE(loaded.callCatering, false);
    QCOMPARE(loaded.callLavatory, false);
    QCOMPARE(loaded.callWater, false);
    QCOMPARE(loaded.callCleaning, false);
    QCOMPARE(loaded.openGsxOnRequests, true);
    QCOMPARE(loaded.themeMode, 2);
    QCOMPARE(loaded.language, std::string("system"));
    QCOMPARE(loaded.updateMode, 1);
    QCOMPARE(loaded.closeToTray, false);
    QCOMPARE(loaded.minimizeToTray, true);
    QCOMPARE(loaded.trayTipShown, false);
    QCOMPARE(loaded.streamerMode, false);
    QVERIFY(loaded.profiles.empty());
}

void QSettingsRepositoryTest::saveLoadRoundTrip()
{
    AppSettings values;
    values.simbriefPilotId = 12345;
    values.streamerMode = true;
    values.fuelRateKgs = 42.5;
    values.autoSelectGsxChoice = false;
    values.autoDeice = true;
    values.crewBoarding = 1;
    values.autoStartFlow = true;
    values.autoStartLoading = false;
    values.skipReposition = true;
    values.callGpu = true;
    values.callGpuOnArrival = true;
    values.callCatering = true;
    values.callLavatory = true;
    values.callWater = true;
    values.callCleaning = true;
    values.openGsxOnRequests = false;
    values.themeMode = 0;
    values.language = "pt_BR";
    values.updateMode = 2;
    values.closeToTray = true;
    values.minimizeToTray = false;
    values.trayTipShown = true;

    AircraftProfile profile;
    profile.useGlobal = false;
    profile.fuelRateKgs = 33.0;
    profile.skipReposition = true;
    profile.callGpu = true;
    profile.callGpuOnArrival = false;
    profile.callCatering = true;
    profile.callLavatory = false;
    profile.callWater = true;
    profile.callCleaning = false;
    values.profiles.emplace("a340", profile);

    QVERIFY(repository_.Save(values));

    const AppSettings loaded = repository_.Load();

    QCOMPARE(loaded.simbriefPilotId, values.simbriefPilotId);
    QCOMPARE(loaded.streamerMode, values.streamerMode);
    QCOMPARE(loaded.fuelRateKgs, values.fuelRateKgs);
    QCOMPARE(loaded.autoSelectGsxChoice, values.autoSelectGsxChoice);
    QCOMPARE(loaded.autoDeice, values.autoDeice);
    QCOMPARE(loaded.crewBoarding, values.crewBoarding);
    QCOMPARE(loaded.autoStartFlow, values.autoStartFlow);
    QCOMPARE(loaded.autoStartLoading, values.autoStartLoading);
    QCOMPARE(loaded.skipReposition, values.skipReposition);
    QCOMPARE(loaded.callGpu, values.callGpu);
    QCOMPARE(loaded.callGpuOnArrival, values.callGpuOnArrival);
    QCOMPARE(loaded.callCatering, values.callCatering);
    QCOMPARE(loaded.callLavatory, values.callLavatory);
    QCOMPARE(loaded.callWater, values.callWater);
    QCOMPARE(loaded.callCleaning, values.callCleaning);
    QCOMPARE(loaded.openGsxOnRequests, values.openGsxOnRequests);
    QCOMPARE(loaded.themeMode, values.themeMode);
    QCOMPARE(loaded.language, values.language);
    QCOMPARE(loaded.updateMode, values.updateMode);
    QCOMPARE(loaded.closeToTray, values.closeToTray);
    QCOMPARE(loaded.minimizeToTray, values.minimizeToTray);
    QCOMPARE(loaded.trayTipShown, values.trayTipShown);

    QCOMPARE(loaded.profiles.size(), std::size_t{1});
    const auto it = loaded.profiles.find("a340");
    QVERIFY(it != loaded.profiles.end());
    QCOMPARE(it->second.useGlobal, profile.useGlobal);
    QCOMPARE(it->second.fuelRateKgs, profile.fuelRateKgs);
    QCOMPARE(it->second.skipReposition, profile.skipReposition);
    QCOMPARE(it->second.callGpu, profile.callGpu);
    QCOMPARE(it->second.callGpuOnArrival, profile.callGpuOnArrival);
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

QTEST_GUILESS_MAIN(QSettingsRepositoryTest)

#include "tst_qsettings_repository.moc"
