#include <QtCore/QLocale>
#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/viewmodel/SettingsViewModel.h"

class SettingsViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void loadsAndAppliesStoredSettings();
    static void rejectsInvalidDraft();
    static void savesValidDraft();
    static void persistsAutoStartFlowImmediately();
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
    static void parsesFuelRateWithCommaDecimal();
    static void parsesFuelRateWithDotDecimal();
    static void seededFuelRateRoundTripsThroughParse();
};

void SettingsViewModelTest::loadsAndAppliesStoredSettings()
{
    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.simbriefPilotId = 42;
    repository.stored.fuelRateKgs = 150.0;
    repository.stored.autoSelectGsxChoice = false;
    repository.stored.themeMode = 1;

    SettingsViewModel viewModel(&repository, &service);

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
    SettingsViewModel viewModel(&repository, &service);

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
    const auto previous = QLocale();
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

    FakeSettingsRepository repository;
    FakeIntegratorService service;

    repository.stored.fuelRateKgs = 1234.5;

    SettingsViewModel viewModel(&repository, &service);

    const QString seeded = viewModel.GetFuelRateText();
    QCOMPARE(seeded, QLocale().toString(1234.5, 'g', 12).remove(QLocale().groupSeparator()));

    viewModel.SetFuelRateText(seeded);
    viewModel.SetSimbriefPilotIdText(QStringLiteral("1"));
    QVERIFY(viewModel.save());
    QCOMPARE(repository.stored.fuelRateKgs, 1234.5);

    QLocale::setDefault(previous);
}

QTEST_APPLESS_MAIN(SettingsViewModelTest)

#include "tst_settings_viewmodel.moc"
