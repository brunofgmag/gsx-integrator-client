#include <QtCore/QLocale>
#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/viewmodel/SettingsViewModel.h"
#include "../src/domain/support/Weight.h"

namespace
{
    std::vector<AircraftProfileInfo> TestProfileInfos()
    {
        return {
            {"toliss-a340", "A346", "ToLiss A340-600", RefuelBy::Self},
            {"ifly-737max8", "B38M", "iFly 737 MAX 8", RefuelBy::Gsx},
            {"fictional-client", "CLI1", "Client Fueled Test Aircraft", RefuelBy::Client},
        };
    }
}

class SettingsViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void loadsAndAppliesStoredSettings();
    static void rejectsInvalidDraft();
    static void savesValidDraft();
    static void persistsAutoStartFlowImmediately();
    static void autoStartLoadingDefaultsToEnabled();
    static void persistsAutoStartLoadingImmediately();
    static void skipRepositionDefaultsToDisabled();
    static void persistsSkipRepositionImmediately();
    static void groundServicesDefaultToDisabled();
    static void groundServicesPersistImmediately();
    static void traySettingsDefaults();
    static void traySettingsPersistImmediately();
    static void streamerModeDefaultsToDisabled();
    static void streamerModePersistsImmediately();
    static void openGsxOnRequestsDefaultsToEnabledAndPersists();
    static void rejectsNonNumericFuelRate();
    static void rejectsNonPositiveFuelRate();
    static void emptyPilotIdIsAcceptedAsZero();
    static void canSaveReflectsValidationState();
    static void repositoryFailureSurfacedOnSave();
    static void themeModeAndAutoSelectPersistImmediately();
    static void settingSameThemeModeIsNoOp();
    static void systemThemeUsesInjectedProvider();
    static void languagePersistsImmediately();
    static void updateModeDefaultsToNotify();
    static void updateModePersistsImmediately();
    static void weightUnitModeDefaultsToAutoAndPersists();
    static void fuelRateFollowsWeightUnit();
    static void autoWeightModeFollowsDetectedUnit();
    static void parsesFuelRateWithCommaDecimal();
    static void parsesFuelRateWithDotDecimal();
    static void seededFuelRateRoundTripsThroughParse();
    static void exposesInjectedProfileModel();
    static void selectsDetectedProfileFromSnapshot();
    static void fuelEditabilityAndBadgeFollowRefuelMethod();
    static void profileDefaultsToUseGlobal();
    static void disablingUseGlobalCopiesCurrentGlobals();
    static void profileEditsAreBufferedUntilSave();
    static void profileDraftLoadsFromStoredSettings();
    static void saveWritesOnlyCustomProfiles();
    static void savePreservesUnknownProfileIds();
    static void invalidClientProfileFuelRateBlocksSave();
    static void nonClientProfileFuelRateIsNotValidated();
    static void setProfileAsGlobalDefaultCopiesValues();
    static void setProfileAsGlobalDefaultSkipsFuelForNonClient();
    static void applyProfileToAllProfilesCopiesDraft();
};

void SettingsViewModelTest::loadsAndAppliesStoredSettings()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.simbriefPilotId = 42;
    repository.stored.fuelRateKgs = 150.0;
    repository.stored.autoSelectGsxChoice = false;
    repository.stored.themeMode = 1;

    const SettingsViewModel viewModel(&repository, &service);

    QCOMPARE(viewModel.GetSimbriefPilotIdText(), QStringLiteral("42"));
    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("150"));
    QVERIFY(!viewModel.GetAutoSelectGsxChoice());
    QCOMPARE(viewModel.GetThemeMode(), 1);
    QVERIFY(viewModel.GetEffectiveDark());
    QCOMPARE(service.applySettingsCalls, 1);
    QCOMPARE(service.appliedSettings.simbriefPilotId, 42);
}

void SettingsViewModelTest::rejectsInvalidDraft()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetSimbriefPilotIdText(QStringLiteral("-5"));

    QVERIFY(!viewModel.save());
    QCOMPARE(repository.saveCalls, 0);
    QVERIFY(viewModel.HasSaveError());
    QCOMPARE(viewModel.GetSaveMessage(), QStringLiteral("Enter a valid SimBrief Pilot ID."));
}

void SettingsViewModelTest::savesValidDraft()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetSimbriefPilotIdText(QStringLiteral("12345"));
    viewModel.SetFuelRateText(QStringLiteral("175.5"));

    QVERIFY(viewModel.save());
    QCOMPARE(repository.saveCalls, 1);
    QCOMPARE(repository.stored.simbriefPilotId, 12345);
    QCOMPARE(repository.stored.fuelRateKgs, 175.5);
    QCOMPARE(service.appliedSettings.simbriefPilotId, 12345);
    QCOMPARE(viewModel.GetSaveMessage(), QStringLiteral("Settings saved."));
    QVERIFY(!viewModel.HasSaveError());
}

void SettingsViewModelTest::persistsAutoStartFlowImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.autoStartFlow = false;

    SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.GetAutoStartFlow());

    viewModel.SetAutoStartFlow(true);

    QVERIFY(viewModel.GetAutoStartFlow());
    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(repository.stored.autoStartFlow);
    QVERIFY(service.appliedSettings.autoStartFlow);
}

void SettingsViewModelTest::autoStartLoadingDefaultsToEnabled()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QVERIFY(viewModel.GetAutoStartLoading());
    QVERIFY(service.appliedSettings.autoStartLoading);
}

void SettingsViewModelTest::persistsAutoStartLoadingImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetAutoStartLoading(false);

    QVERIFY(!viewModel.GetAutoStartLoading());
    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(!repository.stored.autoStartLoading);
    QVERIFY(!service.appliedSettings.autoStartLoading);

    const int savesBefore = repository.saveCalls;
    viewModel.SetAutoStartLoading(false);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::skipRepositionDefaultsToDisabled()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.GetSkipReposition());
    QVERIFY(!service.appliedSettings.skipReposition);
}

void SettingsViewModelTest::persistsSkipRepositionImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetSkipReposition(true);

    QVERIFY(viewModel.GetSkipReposition());
    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(repository.stored.skipReposition);
    QVERIFY(service.appliedSettings.skipReposition);

    const int savesBefore = repository.saveCalls;
    viewModel.SetSkipReposition(true);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::groundServicesDefaultToDisabled()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.GetCallGpu());
    QVERIFY(!viewModel.GetCallCatering());
    QVERIFY(!viewModel.GetCallLavatory());
    QVERIFY(!viewModel.GetCallWater());
    QVERIFY(!viewModel.GetCallCleaning());
    QVERIFY(!service.appliedSettings.callGpu);
    QVERIFY(!service.appliedSettings.callCatering);
    QVERIFY(!service.appliedSettings.callLavatory);
    QVERIFY(!service.appliedSettings.callWater);
    QVERIFY(!service.appliedSettings.callCleaning);
}

void SettingsViewModelTest::groundServicesPersistImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetCallGpu(true);

    QVERIFY(viewModel.GetCallGpu());
    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(repository.stored.callGpu);
    QVERIFY(service.appliedSettings.callGpu);

    viewModel.SetCallCatering(true);

    QVERIFY(viewModel.GetCallCatering());
    QCOMPARE(repository.saveCalls, 2);
    QVERIFY(repository.stored.callCatering);
    QVERIFY(service.appliedSettings.callCatering);

    viewModel.SetCallLavatory(true);

    QVERIFY(viewModel.GetCallLavatory());
    QCOMPARE(repository.saveCalls, 3);
    QVERIFY(repository.stored.callLavatory);
    QVERIFY(service.appliedSettings.callLavatory);

    viewModel.SetCallWater(true);

    QVERIFY(viewModel.GetCallWater());
    QCOMPARE(repository.saveCalls, 4);
    QVERIFY(repository.stored.callWater);
    QVERIFY(service.appliedSettings.callWater);

    viewModel.SetCallCleaning(true);

    QVERIFY(viewModel.GetCallCleaning());
    QCOMPARE(repository.saveCalls, 5);
    QVERIFY(repository.stored.callCleaning);
    QVERIFY(service.appliedSettings.callCleaning);

    const int savesBefore = repository.saveCalls;
    viewModel.SetCallGpu(true);
    viewModel.SetCallCatering(true);
    viewModel.SetCallLavatory(true);
    viewModel.SetCallWater(true);
    viewModel.SetCallCleaning(true);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::traySettingsDefaults()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.GetCloseToTray());
    QVERIFY(viewModel.GetMinimizeToTray());
    QVERIFY(!viewModel.GetTrayTipShown());
}

void SettingsViewModelTest::traySettingsPersistImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetCloseToTray(true);

    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(repository.stored.closeToTray);

    viewModel.SetMinimizeToTray(false);

    QCOMPARE(repository.saveCalls, 2);
    QVERIFY(!repository.stored.minimizeToTray);

    viewModel.SetTrayTipShown(true);

    QCOMPARE(repository.saveCalls, 3);
    QVERIFY(repository.stored.trayTipShown);

    const int savesBefore = repository.saveCalls;
    viewModel.SetCloseToTray(true);
    viewModel.SetMinimizeToTray(false);
    viewModel.SetTrayTipShown(true);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::streamerModeDefaultsToDisabled()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.GetStreamerMode());
}

void SettingsViewModelTest::streamerModePersistsImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetStreamerMode(true);

    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(repository.stored.streamerMode);

    const int savesBefore = repository.saveCalls;
    viewModel.SetStreamerMode(true);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::openGsxOnRequestsDefaultsToEnabledAndPersists()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    QVERIFY(viewModel.GetOpenGsxOnRequests());
    QVERIFY(service.appliedSettings.openGsxOnRequests);

    viewModel.SetOpenGsxOnRequests(false);

    QVERIFY(!viewModel.GetOpenGsxOnRequests());
    QCOMPARE(repository.saveCalls, 1);
    QVERIFY(!repository.stored.openGsxOnRequests);
    QVERIFY(!service.appliedSettings.openGsxOnRequests);

    const int savesBefore = repository.saveCalls;
    viewModel.SetOpenGsxOnRequests(false);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::rejectsNonNumericFuelRate()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetFuelRateText(QStringLiteral("abc"));

    QVERIFY(!viewModel.save());
    QVERIFY(viewModel.HasSaveError());
    QCOMPARE(viewModel.GetSaveMessage(), QStringLiteral("Enter a valid fuel rate."));
}

void SettingsViewModelTest::rejectsNonPositiveFuelRate()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetFuelRateText(QStringLiteral("0"));

    QVERIFY(!viewModel.save());

    viewModel.SetFuelRateText(QStringLiteral("-25"));

    QVERIFY(!viewModel.save());
}

void SettingsViewModelTest::emptyPilotIdIsAcceptedAsZero()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.fuelRateKgs = 60.0;

    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetSimbriefPilotIdText(QString());

    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.simbriefPilotId, 0);
}

void SettingsViewModelTest::canSaveReflectsValidationState()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.fuelRateKgs = 60.0;

    SettingsViewModel viewModel(&repository, &service);

    QVERIFY(viewModel.CanSave());

    viewModel.SetFuelRateText(QStringLiteral("nope"));

    QVERIFY(!viewModel.CanSave());
    QCOMPARE(viewModel.GetValidationMessage(), QStringLiteral("Enter a valid fuel rate."));
}

void SettingsViewModelTest::repositoryFailureSurfacedOnSave()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.fuelRateKgs = 60.0;
    repository.saveResult = false;

    SettingsViewModel viewModel(&repository, &service);

    QVERIFY(!viewModel.save());
    QVERIFY(viewModel.HasSaveError());
    QCOMPARE(viewModel.GetSaveMessage(), QStringLiteral("Could not save settings."));
}

void SettingsViewModelTest::themeModeAndAutoSelectPersistImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.themeMode = 0;
    repository.stored.autoSelectGsxChoice = true;

    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetThemeMode(1);

    QCOMPARE(repository.stored.themeMode, 1);
    QCOMPARE(service.appliedSettings.themeMode, 1);
    QVERIFY(viewModel.GetEffectiveDark());

    viewModel.SetAutoSelectGsxChoice(false);

    QVERIFY(!repository.stored.autoSelectGsxChoice);
    QVERIFY(!service.appliedSettings.autoSelectGsxChoice);
}

void SettingsViewModelTest::settingSameThemeModeIsNoOp()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.themeMode = 1;

    SettingsViewModel viewModel(&repository, &service);

    const int savesBefore = repository.saveCalls;
    viewModel.SetThemeMode(1);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::systemThemeUsesInjectedProvider()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.themeMode = 2;

    SettingsViewModel viewModel(&repository, &service);
    QVERIFY(!viewModel.GetEffectiveDark());

    bool systemDark = true;
    viewModel.SetSystemDarkProvider([&systemDark] { return systemDark; });
    QVERIFY(viewModel.GetEffectiveDark());

    systemDark = false;
    viewModel.RefreshEffectiveTheme();
    QVERIFY(!viewModel.GetEffectiveDark());
}

void SettingsViewModelTest::languagePersistsImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    QCOMPARE(viewModel.GetLanguage(), QStringLiteral("system"));

    viewModel.SetLanguage(QStringLiteral("pt_BR"));

    QVERIFY(repository.stored.language == "pt_BR");
    QVERIFY(service.appliedSettings.language == "pt_BR");
    QCOMPARE(viewModel.GetLanguage(), QStringLiteral("pt_BR"));
}

void SettingsViewModelTest::updateModeDefaultsToNotify()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service);

    QCOMPARE(viewModel.GetUpdateMode(), static_cast<int>(SettingsViewModel::Notify));
}

void SettingsViewModelTest::updateModePersistsImmediately()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetUpdateMode(SettingsViewModel::Auto);

    QCOMPARE(repository.saveCalls, 1);
    QCOMPARE(repository.stored.updateMode, 0);

    const int savesBefore = repository.saveCalls;
    viewModel.SetUpdateMode(SettingsViewModel::Auto);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::weightUnitModeDefaultsToAutoAndPersists()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    QCOMPARE(viewModel.GetWeightUnitMode(), static_cast<int>(SettingsViewModel::AutoUnit));

    viewModel.SetWeightUnitMode(SettingsViewModel::Pounds);

    QCOMPARE(viewModel.GetWeightUnitMode(), static_cast<int>(SettingsViewModel::Pounds));
    QCOMPARE(repository.saveCalls, 1);
    QCOMPARE(repository.stored.weightUnitMode, static_cast<int>(SettingsViewModel::Pounds));

    const int savesBefore = repository.saveCalls;
    viewModel.SetWeightUnitMode(SettingsViewModel::Pounds);

    QCOMPARE(repository.saveCalls, savesBefore);
}

void SettingsViewModelTest::fuelRateFollowsWeightUnit()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    repository.stored.fuelRateKgs = 100.0;
    SettingsViewModel viewModel(&repository, &service);

    // Auto with no detected unit resolves to kilograms.
    QVERIFY(!viewModel.GetWeightIsLb());
    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("100"));

    // Switching to pounds shows a whole-number rate (no long decimal tail).
    viewModel.SetWeightUnitMode(SettingsViewModel::Pounds);
    QVERIFY(viewModel.GetWeightIsLb());
    QCOMPARE(viewModel.GetFuelRateUnitText(), QStringLiteral("lb/s"));
    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("220"));
    QVERIFY(viewModel.save());
    QVERIFY(qAbs(repository.stored.fuelRateKgs - weight::LbToKg(220.0)) < 1e-9);

    // Switching back to kilograms shows a whole number again.
    viewModel.SetWeightUnitMode(SettingsViewModel::Kilograms);
    QVERIFY(!viewModel.GetWeightIsLb());
    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("100"));
    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.fuelRateKgs, 100.0);
}

void SettingsViewModelTest::autoWeightModeFollowsDetectedUnit()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    service.snapshot.autoWeightUnit = 1; // Lb
    repository.stored.fuelRateKgs = 100.0;
    SettingsViewModel viewModel(&repository, &service);

    // Auto mode + detected pounds -> the rate buffer seeds in whole lb/s.
    QVERIFY(viewModel.GetWeightIsLb());
    QCOMPARE(viewModel.GetFuelRateUnitText(), QStringLiteral("lb/s"));
    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("220"));
    QVERIFY(viewModel.save());
    QVERIFY(qAbs(repository.stored.fuelRateKgs - weight::LbToKg(220.0)) < 1e-9);
}

void SettingsViewModelTest::parsesFuelRateWithCommaDecimal()
{
    const auto previous = QLocale();
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetFuelRateText(QStringLiteral("60,5"));

    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.fuelRateKgs, 60.5);

    QLocale::setDefault(previous);
}

void SettingsViewModelTest::parsesFuelRateWithDotDecimal()
{
    const auto previous = QLocale();
    QLocale::setDefault(QLocale::c());

    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service);

    viewModel.SetFuelRateText(QStringLiteral("60.5"));

    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.fuelRateKgs, 60.5);

    QLocale::setDefault(previous);
}

void SettingsViewModelTest::seededFuelRateRoundTripsThroughParse()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.fuelRateKgs = 1234.0;

    SettingsViewModel viewModel(&repository, &service);

    const QString seeded = viewModel.GetFuelRateText();
    QCOMPARE(seeded, QStringLiteral("1234"));

    viewModel.SetFuelRateText(seeded);
    viewModel.SetSimbriefPilotIdText(QStringLiteral("1"));
    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.fuelRateKgs, 1234.0);
}

void SettingsViewModelTest::exposesInjectedProfileModel()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    const QVariantList model = viewModel.GetProfileModel();

    QCOMPARE(model.size(), 3);
    QCOMPARE(model[0].toMap().value("shortCode").toString(), QStringLiteral("A346"));
    QCOMPARE(model[2].toMap().value("name").toString(),
             QStringLiteral("Client Fueled Test Aircraft"));
    QCOMPARE(viewModel.GetSelectedProfileIndex(), 0);
    QCOMPARE(viewModel.GetDetectedProfileIndex(), -1);
}

void SettingsViewModelTest::selectsDetectedProfileFromSnapshot()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    service.snapshot.aircraftProfileId = "ifly-737max8";
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    QCOMPARE(viewModel.GetDetectedProfileIndex(), 1);
    QCOMPARE(viewModel.GetSelectedProfileIndex(), 1);

    service.snapshot.aircraftProfileId = "toliss-a340";
    viewModel.selectDetectedProfile();

    QCOMPARE(viewModel.GetDetectedProfileIndex(), 0);
    QCOMPARE(viewModel.GetSelectedProfileIndex(), 0);
}

void SettingsViewModelTest::fuelEditabilityAndBadgeFollowRefuelMethod()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(0);
    QVERIFY(!viewModel.GetProfileFuelEditable());
    QCOMPARE(viewModel.GetProfileFuelBadge(), QStringLiteral("GSX"));

    viewModel.SetSelectedProfileIndex(1);
    QVERIFY(!viewModel.GetProfileFuelEditable());
    QCOMPARE(viewModel.GetProfileFuelBadge(), QStringLiteral("Auto"));

    viewModel.SetSelectedProfileIndex(2);
    QVERIFY(viewModel.GetProfileFuelEditable());
    QCOMPARE(viewModel.GetProfileFuelBadge(), QString());
}

void SettingsViewModelTest::profileDefaultsToUseGlobal()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    const SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    QVERIFY(viewModel.GetProfileUseGlobal());
}

void SettingsViewModelTest::disablingUseGlobalCopiesCurrentGlobals()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    repository.stored.fuelRateKgs = 150.0;
    repository.stored.callGpu = true;
    repository.stored.skipReposition = true;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(2);
    viewModel.SetProfileUseGlobal(false);

    QVERIFY(!viewModel.GetProfileUseGlobal());
    QCOMPARE(viewModel.GetProfileFuelRateText(), QStringLiteral("150"));
    QVERIFY(viewModel.GetProfileCallGpu());
    QVERIFY(viewModel.GetProfileSkipReposition());
    QVERIFY(!viewModel.GetProfileCallCatering());
}

void SettingsViewModelTest::profileEditsAreBufferedUntilSave()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());
    const int savesAfterConstruction = repository.saveCalls;

    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileCallCatering(true);
    viewModel.SetProfileCallWater(true);

    QCOMPARE(repository.saveCalls, savesAfterConstruction);
    QVERIFY(viewModel.GetProfileCallCatering());
    QVERIFY(viewModel.GetProfileCallWater());
}

void SettingsViewModelTest::profileDraftLoadsFromStoredSettings()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    AircraftProfile stored;
    stored.useGlobal = false;
    stored.fuelRateKgs = 12.0;
    stored.callLavatory = true;
    repository.stored.profiles.emplace("fictional-client", stored);
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(2);

    QVERIFY(!viewModel.GetProfileUseGlobal());
    QCOMPARE(viewModel.GetProfileFuelRateText(), QStringLiteral("12"));
    QVERIFY(viewModel.GetProfileCallLavatory());
}

void SettingsViewModelTest::saveWritesOnlyCustomProfiles()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(2);
    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileFuelRateText(QStringLiteral("12.5"));
    viewModel.SetProfileCallCleaning(true);

    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.profiles.size(), 1u);
    const AircraftProfile& saved = repository.stored.profiles.at("fictional-client");
    QVERIFY(!saved.useGlobal);
    QCOMPARE(saved.fuelRateKgs, 12.5);
    QVERIFY(saved.callCleaning);
    QCOMPARE(service.appliedSettings.profiles.size(), 1u);
}

void SettingsViewModelTest::savePreservesUnknownProfileIds()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    AircraftProfile orphan;
    orphan.useGlobal = false;
    orphan.callGpu = true;
    repository.stored.profiles.emplace("removed-aircraft", orphan);
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.profiles.size(), 1u);
    QVERIFY(repository.stored.profiles.at("removed-aircraft").callGpu);
}

void SettingsViewModelTest::invalidClientProfileFuelRateBlocksSave()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(2);
    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileFuelRateText(QStringLiteral("zero"));

    QVERIFY(!viewModel.CanSave());
    QVERIFY(!viewModel.save());
    QCOMPARE(viewModel.GetSaveMessage(), QStringLiteral("Enter a valid fuel rate for CLI1."));
}

void SettingsViewModelTest::nonClientProfileFuelRateIsNotValidated()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());

    viewModel.SetSelectedProfileIndex(1);
    viewModel.SetProfileUseGlobal(false);

    QVERIFY(viewModel.CanSave());
    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.profiles.at("ifly-737max8").fuelRateKgs,
             repository.stored.fuelRateKgs);
}

void SettingsViewModelTest::setProfileAsGlobalDefaultCopiesValues()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());
    const int savesAfterConstruction = repository.saveCalls;

    viewModel.SetSelectedProfileIndex(2);
    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileFuelRateText(QStringLiteral("33"));
    viewModel.SetProfileCallWater(true);
    viewModel.setProfileAsGlobalDefault();

    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("33"));
    QVERIFY(viewModel.GetCallWater());
    QVERIFY(viewModel.GetProfileUseGlobal());
    QCOMPARE(repository.saveCalls, savesAfterConstruction);
}

void SettingsViewModelTest::setProfileAsGlobalDefaultSkipsFuelForNonClient()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    repository.stored.fuelRateKgs = 60.0;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());
    const int savesAfterConstruction = repository.saveCalls;

    viewModel.SetSelectedProfileIndex(0);
    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileCallGpu(true);
    viewModel.setProfileAsGlobalDefault();

    QCOMPARE(viewModel.GetFuelRateText(), QStringLiteral("60"));
    QVERIFY(viewModel.GetCallGpu());
    QCOMPARE(repository.saveCalls, savesAfterConstruction);
}

void SettingsViewModelTest::applyProfileToAllProfilesCopiesDraft()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;
    SettingsViewModel viewModel(&repository, &service, TestProfileInfos());
    const int savesAfterConstruction = repository.saveCalls;

    viewModel.SetSelectedProfileIndex(2);
    viewModel.SetProfileUseGlobal(false);
    viewModel.SetProfileCallCleaning(true);
    viewModel.applyProfileToAllProfiles();

    viewModel.SetSelectedProfileIndex(0);
    QVERIFY(!viewModel.GetProfileUseGlobal());
    QVERIFY(viewModel.GetProfileCallCleaning());
    viewModel.SetSelectedProfileIndex(1);
    QVERIFY(!viewModel.GetProfileUseGlobal());
    QVERIFY(viewModel.GetProfileCallCleaning());
    QCOMPARE(repository.saveCalls, savesAfterConstruction);
}

QTEST_APPLESS_MAIN(SettingsViewModelTest)

#include "tst_settings_viewmodel.moc"
