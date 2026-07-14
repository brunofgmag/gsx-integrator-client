#include "QSettingsRepository.h"

namespace
{
    constexpr auto kKeySimbriefPilotId = "simbrief/pilotId";
    constexpr auto kKeyFuelRateKgs = "fuel/rateKgs";
    constexpr auto kKeyAutoSelectGsxChoice = "gsx/autoSelectGsxChoice";
    constexpr auto kKeyAutoStartFlow = "automation/autoStartFlow";
    constexpr auto kKeyAutoStartLoading = "automation/autoStartLoading";
    constexpr auto kKeySkipReposition = "automation/skipReposition";
    constexpr auto kKeyCallGpu = "services/callGpu";
    constexpr auto kKeyCallCatering = "services/callCatering";
    constexpr auto kKeyCallLavatory = "services/callLavatory";
    constexpr auto kKeyCallWater = "services/callWater";
    constexpr auto kKeyCallCleaning = "services/callCleaning";
    constexpr auto kKeyThemeMode = "ui/themeMode";
    constexpr auto kKeyDarkThemeLegacy = "ui/darkTheme";
    constexpr auto kKeyLanguage = "ui/language";
    constexpr auto kKeyUpdateMode = "updates/mode";
    constexpr auto kKeyCloseToTray = "ui/closeToTray";
    constexpr auto kKeyMinimizeToTray = "ui/minimizeToTray";
    constexpr auto kKeyTrayTipShown = "ui/trayTipShown";
    constexpr auto kKeyStreamerMode = "ui/streamerMode";
    constexpr auto kGroupProfiles = "profiles";
    constexpr auto kKeyProfileUseGlobal = "useGlobal";
    constexpr auto kKeyProfileFuelRateKgs = "fuelRateKgs";
    constexpr auto kKeyProfileSkipReposition = "skipReposition";
    constexpr auto kKeyProfileCallGpu = "callGpu";
    constexpr auto kKeyProfileCallCatering = "callCatering";
    constexpr auto kKeyProfileCallLavatory = "callLavatory";
    constexpr auto kKeyProfileCallWater = "callWater";
    constexpr auto kKeyProfileCallCleaning = "callCleaning";
}

AppSettings QSettingsRepository::Load() const
{
    QSettings settings;

    AppSettings result;
    result.simbriefPilotId = settings.value(kKeySimbriefPilotId, 0).toInt();
    result.fuelRateKgs = settings.value(kKeyFuelRateKgs, 60.0).toDouble();
    result.autoSelectGsxChoice = settings.value(kKeyAutoSelectGsxChoice, true).toBool();
    result.autoStartFlow = settings.value(kKeyAutoStartFlow, false).toBool();
    result.autoStartLoading = settings.value(kKeyAutoStartLoading, true).toBool();
    result.skipReposition = settings.value(kKeySkipReposition, false).toBool();
    result.callGpu = settings.value(kKeyCallGpu, false).toBool();
    result.callCatering = settings.value(kKeyCallCatering, false).toBool();
    result.callLavatory = settings.value(kKeyCallLavatory, false).toBool();
    result.callWater = settings.value(kKeyCallWater, false).toBool();
    result.callCleaning = settings.value(kKeyCallCleaning, false).toBool();

    int themeMode = settings.value(kKeyThemeMode, -1).toInt();
    if (themeMode < 0)
    {
        themeMode = settings.value(kKeyDarkThemeLegacy, false).toBool() ? 1 : 2;
    }
    result.themeMode = themeMode;

    result.language = settings.value(kKeyLanguage, "system").toString().toStdString();
    result.updateMode = settings.value(kKeyUpdateMode, 1).toInt();
    result.closeToTray = settings.value(kKeyCloseToTray, true).toBool();
    result.minimizeToTray = settings.value(kKeyMinimizeToTray, true).toBool();
    result.trayTipShown = settings.value(kKeyTrayTipShown, false).toBool();
    result.streamerMode = settings.value(kKeyStreamerMode, false).toBool();

    settings.beginGroup(kGroupProfiles);
    const QStringList profileIds = settings.childGroups();
    for (const QString& profileId : profileIds)
    {
        settings.beginGroup(profileId);
        AircraftProfile profile;
        profile.useGlobal = settings.value(kKeyProfileUseGlobal, false).toBool();
        profile.fuelRateKgs = settings.value(kKeyProfileFuelRateKgs,
                                             AutomationSettings::kDefaultFuelRateKgs).toDouble();
        profile.skipReposition = settings.value(kKeyProfileSkipReposition, false).toBool();
        profile.callGpu = settings.value(kKeyProfileCallGpu, false).toBool();
        profile.callCatering = settings.value(kKeyProfileCallCatering, false).toBool();
        profile.callLavatory = settings.value(kKeyProfileCallLavatory, false).toBool();
        profile.callWater = settings.value(kKeyProfileCallWater, false).toBool();
        profile.callCleaning = settings.value(kKeyProfileCallCleaning, false).toBool();
        result.profiles.emplace(profileId.toStdString(), profile);
        settings.endGroup();
    }
    settings.endGroup();

    return result;
}

bool QSettingsRepository::Save(const AppSettings& values)
{
    QSettings settings;
    settings.setValue(kKeySimbriefPilotId, values.simbriefPilotId);
    settings.setValue(kKeyFuelRateKgs, values.fuelRateKgs);
    settings.setValue(kKeyAutoSelectGsxChoice, values.autoSelectGsxChoice);
    settings.setValue(kKeyAutoStartFlow, values.autoStartFlow);
    settings.setValue(kKeyAutoStartLoading, values.autoStartLoading);
    settings.setValue(kKeySkipReposition, values.skipReposition);
    settings.setValue(kKeyCallGpu, values.callGpu);
    settings.setValue(kKeyCallCatering, values.callCatering);
    settings.setValue(kKeyCallLavatory, values.callLavatory);
    settings.setValue(kKeyCallWater, values.callWater);
    settings.setValue(kKeyCallCleaning, values.callCleaning);
    settings.setValue(kKeyThemeMode, values.themeMode);
    settings.setValue(kKeyLanguage, QString::fromStdString(values.language));
    settings.setValue(kKeyUpdateMode, values.updateMode);
    settings.setValue(kKeyCloseToTray, values.closeToTray);
    settings.setValue(kKeyMinimizeToTray, values.minimizeToTray);
    settings.setValue(kKeyTrayTipShown, values.trayTipShown);
    settings.setValue(kKeyStreamerMode, values.streamerMode);

    settings.beginGroup(kGroupProfiles);
    settings.remove("");
    for (const auto& [profileId, profile] : values.profiles)
    {
        if (profile.useGlobal)
        {
            continue;
        }
        settings.beginGroup(QString::fromStdString(profileId));
        settings.setValue(kKeyProfileUseGlobal, profile.useGlobal);
        settings.setValue(kKeyProfileFuelRateKgs, profile.fuelRateKgs);
        settings.setValue(kKeyProfileSkipReposition, profile.skipReposition);
        settings.setValue(kKeyProfileCallGpu, profile.callGpu);
        settings.setValue(kKeyProfileCallCatering, profile.callCatering);
        settings.setValue(kKeyProfileCallLavatory, profile.callLavatory);
        settings.setValue(kKeyProfileCallWater, profile.callWater);
        settings.setValue(kKeyProfileCallCleaning, profile.callCleaning);
        settings.endGroup();
    }
    settings.endGroup();

    settings.sync();

    return settings.status() == QSettings::NoError;
}
