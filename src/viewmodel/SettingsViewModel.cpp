#include "SettingsViewModel.h"

#include <QtCore/QLocale>
#include <utility>
#include "../application/ports/IntegratorService.h"
#include "../application/ports/SettingsRepository.h"

namespace
{
    double ParseFuelRate(const QString& text, bool* ok)
    {
        const QString trimmed = text.trimmed();
        const double value = QLocale().toDouble(trimmed, ok);
        if (*ok)
        {
            return value;
        }
        return QLocale::c().toDouble(trimmed, ok);
    }

    QString FormatFuelRate(const double value)
    {
        return QLocale().toString(value, 'g', 12).remove(QLocale().groupSeparator());
    }
}

SettingsViewModel::SettingsViewModel(
    SettingsRepository* repository,
    IntegratorService* integratorService,
    std::vector<AircraftProfileInfo> profileInfos,
    QObject* parent)
    : QObject(parent),
      repository_(repository),
      integratorService_(integratorService),
      settings_(repository_->Load()),
      profileInfos_(std::move(profileInfos))
{
    simbriefPilotIdText_ = settings_.simbriefPilotId > 0
                               ? QString::number(settings_.simbriefPilotId)
                               : QString();
    fuelRateText_ = FormatFuelRate(settings_.fuelRateKgs);
    integratorService_->ApplySettings(settings_);

    for (const AircraftProfileInfo& info : profileInfos_)
    {
        ProfileDraft draft;
        const auto it = settings_.profiles.find(info.id);
        if (it != settings_.profiles.end())
        {
            draft.useGlobal = it->second.useGlobal;
            draft.fuelRateText = FormatFuelRate(it->second.fuelRateKgs);
            draft.skipReposition = it->second.skipReposition;
            draft.callGpu = it->second.callGpu;
            draft.callCatering = it->second.callCatering;
            draft.callLavatory = it->second.callLavatory;
            draft.callWater = it->second.callWater;
            draft.callCleaning = it->second.callCleaning;
        }
        else
        {
            draft.fuelRateText = fuelRateText_;
        }
        profileDrafts_.push_back(draft);
    }

    selectDetectedProfile();
}

QString SettingsViewModel::GetSimbriefPilotIdText() const
{
    return simbriefPilotIdText_;
}

void SettingsViewModel::SetSimbriefPilotIdText(const QString& pilotId)
{
    if (simbriefPilotIdText_ == pilotId)
    {
        return;
    }

    simbriefPilotIdText_ = pilotId;

    SetSaveResult({}, false);

    emit SimbriefPilotIdTextChanged();
    emit ValidationChanged();
}

QString SettingsViewModel::GetFuelRateText() const
{
    return fuelRateText_;
}

void SettingsViewModel::SetFuelRateText(const QString& rate)
{
    if (fuelRateText_ == rate)
    {
        return;
    }

    fuelRateText_ = rate;

    SetSaveResult({}, false);

    emit FuelRateTextChanged();
    emit ValidationChanged();
}

bool SettingsViewModel::GetAutoSelectGsxChoice() const
{
    return settings_.autoSelectGsxChoice;
}

void SettingsViewModel::SetAutoSelectGsxChoice(const bool enabled)
{
    if (settings_.autoSelectGsxChoice == enabled)
    {
        return;
    }

    settings_.autoSelectGsxChoice = enabled;

    PersistImmediateSetting();

    emit AutoSelectGsxChoiceChanged();
}

bool SettingsViewModel::GetAutoStartFlow() const
{
    return settings_.autoStartFlow;
}

void SettingsViewModel::SetAutoStartFlow(const bool enabled)
{
    if (settings_.autoStartFlow == enabled)
    {
        return;
    }

    settings_.autoStartFlow = enabled;

    PersistImmediateSetting();

    emit AutoStartFlowChanged();
}

bool SettingsViewModel::GetAutoStartLoading() const
{
    return settings_.autoStartLoading;
}

void SettingsViewModel::SetAutoStartLoading(const bool enabled)
{
    if (settings_.autoStartLoading == enabled)
    {
        return;
    }

    settings_.autoStartLoading = enabled;

    PersistImmediateSetting();

    emit AutoStartLoadingChanged();
}

bool SettingsViewModel::GetSkipReposition() const
{
    return settings_.skipReposition;
}

void SettingsViewModel::SetSkipReposition(const bool enabled)
{
    if (settings_.skipReposition == enabled)
    {
        return;
    }

    settings_.skipReposition = enabled;

    PersistImmediateSetting();

    emit SkipRepositionChanged();
}

bool SettingsViewModel::GetCallGpu() const
{
    return settings_.callGpu;
}

void SettingsViewModel::SetCallGpu(const bool enabled)
{
    if (settings_.callGpu == enabled)
    {
        return;
    }

    settings_.callGpu = enabled;

    PersistImmediateSetting();

    emit CallGpuChanged();
}

bool SettingsViewModel::GetCallCatering() const
{
    return settings_.callCatering;
}

void SettingsViewModel::SetCallCatering(const bool enabled)
{
    if (settings_.callCatering == enabled)
    {
        return;
    }

    settings_.callCatering = enabled;

    PersistImmediateSetting();

    emit CallCateringChanged();
}

bool SettingsViewModel::GetCallLavatory() const
{
    return settings_.callLavatory;
}

void SettingsViewModel::SetCallLavatory(const bool enabled)
{
    if (settings_.callLavatory == enabled)
    {
        return;
    }

    settings_.callLavatory = enabled;

    PersistImmediateSetting();

    emit CallLavatoryChanged();
}

bool SettingsViewModel::GetCallWater() const
{
    return settings_.callWater;
}

void SettingsViewModel::SetCallWater(const bool enabled)
{
    if (settings_.callWater == enabled)
    {
        return;
    }

    settings_.callWater = enabled;

    PersistImmediateSetting();

    emit CallWaterChanged();
}

bool SettingsViewModel::GetCallCleaning() const
{
    return settings_.callCleaning;
}

void SettingsViewModel::SetCallCleaning(const bool enabled)
{
    if (settings_.callCleaning == enabled)
    {
        return;
    }

    settings_.callCleaning = enabled;

    PersistImmediateSetting();

    emit CallCleaningChanged();
}

int SettingsViewModel::GetThemeMode() const
{
    return settings_.themeMode;
}

void SettingsViewModel::SetThemeMode(const int mode)
{
    if (settings_.themeMode == mode)
    {
        return;
    }

    settings_.themeMode = mode;

    PersistImmediateSetting();

    emit ThemeModeChanged();
    emit EffectiveDarkChanged();
}

bool SettingsViewModel::GetEffectiveDark() const
{
    switch (settings_.themeMode)
    {
    case Light:
        return false;
    case Dark:
        return true;
    default:
        return systemDarkProvider_ ? systemDarkProvider_() : false;
    }
}

void SettingsViewModel::SetSystemDarkProvider(std::function<bool()> provider)
{
    systemDarkProvider_ = std::move(provider);

    emit EffectiveDarkChanged();
}

void SettingsViewModel::RefreshEffectiveTheme()
{
    emit EffectiveDarkChanged();
}

QString SettingsViewModel::GetLanguage() const
{
    return QString::fromStdString(settings_.language);
}

void SettingsViewModel::SetLanguage(const QString& language)
{
    const std::string value = language.toStdString();
    if (settings_.language == value)
    {
        return;
    }

    settings_.language = value;

    PersistImmediateSetting();

    emit LanguageChanged();
}

int SettingsViewModel::GetUpdateMode() const
{
    return settings_.updateMode;
}

void SettingsViewModel::SetUpdateMode(const int mode)
{
    if (settings_.updateMode == mode)
    {
        return;
    }

    settings_.updateMode = mode;

    PersistImmediateSetting();

    emit UpdateModeChanged();
}

bool SettingsViewModel::GetCloseToTray() const
{
    return settings_.closeToTray;
}

void SettingsViewModel::SetCloseToTray(const bool enabled)
{
    if (settings_.closeToTray == enabled)
    {
        return;
    }

    settings_.closeToTray = enabled;

    PersistImmediateSetting();

    emit CloseToTrayChanged();
}

bool SettingsViewModel::GetMinimizeToTray() const
{
    return settings_.minimizeToTray;
}

void SettingsViewModel::SetMinimizeToTray(const bool enabled)
{
    if (settings_.minimizeToTray == enabled)
    {
        return;
    }

    settings_.minimizeToTray = enabled;

    PersistImmediateSetting();

    emit MinimizeToTrayChanged();
}

bool SettingsViewModel::GetTrayTipShown() const
{
    return settings_.trayTipShown;
}

void SettingsViewModel::SetTrayTipShown(const bool shown)
{
    if (settings_.trayTipShown == shown)
    {
        return;
    }

    settings_.trayTipShown = shown;

    PersistImmediateSetting();

    emit TrayTipShownChanged();
}

void SettingsViewModel::RetranslateUi()
{
    emit ValidationChanged();
    emit ProfileSelectionChanged();
}

bool SettingsViewModel::CanSave() const
{
    return Validate().valid;
}

QString SettingsViewModel::GetValidationMessage() const
{
    return Validate().error;
}

QString SettingsViewModel::GetSaveMessage() const
{
    return saveMessage_;
}

bool SettingsViewModel::HasSaveError() const
{
    return saveError_;
}

bool SettingsViewModel::save()
{
    const auto [
        pilotId,
        fuelRateKgs,
        profileFuelRates,
        valid,
        error
    ] = Validate();

    if (!valid)
    {
        SetSaveResult(error, true);

        return false;
    }

    settings_.simbriefPilotId = pilotId;
    settings_.fuelRateKgs = fuelRateKgs;

    for (const AircraftProfileInfo& info : profileInfos_)
    {
        settings_.profiles.erase(info.id);
    }
    for (size_t i = 0; i < profileInfos_.size(); ++i)
    {
        const ProfileDraft& draft = profileDrafts_[i];
        if (draft.useGlobal)
        {
            continue;
        }

        AircraftProfile profile;
        profile.useGlobal = false;
        const auto rate = profileFuelRates.find(profileInfos_[i].id);
        profile.fuelRateKgs = rate != profileFuelRates.end() ? rate->second : settings_.fuelRateKgs;
        profile.skipReposition = draft.skipReposition;
        profile.callGpu = draft.callGpu;
        profile.callCatering = draft.callCatering;
        profile.callLavatory = draft.callLavatory;
        profile.callWater = draft.callWater;
        profile.callCleaning = draft.callCleaning;
        settings_.profiles[profileInfos_[i].id] = profile;
    }

    if (!repository_->Save(settings_))
    {
        SetSaveResult(tr("Could not save settings."), true);

        return false;
    }

    integratorService_->ApplySettings(settings_);
    SetSaveResult(tr("Settings saved."), false);

    return true;
}

void SettingsViewModel::clearSaveMessage()
{
    SetSaveResult({}, false);
}

QVariantList SettingsViewModel::GetProfileModel() const
{
    QVariantList model;
    for (const AircraftProfileInfo& info : profileInfos_)
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("shortCode"), QString::fromStdString(info.shortCode));
        entry.insert(QStringLiteral("name"), QString::fromStdString(info.name));
        model.append(entry);
    }
    return model;
}

int SettingsViewModel::GetSelectedProfileIndex() const
{
    return selectedProfileIndex_;
}

void SettingsViewModel::SetSelectedProfileIndex(const int index)
{
    if (index < 0 || index >= static_cast<int>(profileInfos_.size()) || selectedProfileIndex_ == index)
    {
        return;
    }

    selectedProfileIndex_ = index;

    emit ProfileSelectionChanged();
    emit ProfileDraftChanged();
}

int SettingsViewModel::GetDetectedProfileIndex() const
{
    return detectedProfileIndex_;
}

bool SettingsViewModel::GetProfileFuelEditable() const
{
    return !profileInfos_.empty()
        && profileInfos_[selectedProfileIndex_].refuelBy == RefuelBy::Client;
}

QString SettingsViewModel::GetProfileFuelBadge() const
{
    if (profileInfos_.empty())
    {
        return {};
    }

    switch (profileInfos_[selectedProfileIndex_].refuelBy)
    {
    case RefuelBy::Gsx:
        return tr("Auto");
    case RefuelBy::Self:
        return QStringLiteral("GSX");
    default:
        return {};
    }
}

void SettingsViewModel::selectDetectedProfile()
{
    RefreshDetectedProfile();
    if (detectedProfileIndex_ >= 0 && detectedProfileIndex_ != selectedProfileIndex_)
    {
        selectedProfileIndex_ = detectedProfileIndex_;
    }

    emit ProfileSelectionChanged();
    emit ProfileDraftChanged();
}

void SettingsViewModel::setProfileAsGlobalDefault()
{
    if (profileDrafts_.empty())
    {
        return;
    }

    ProfileDraft& draft = SelectedDraft();
    if (draft.useGlobal)
    {
        return;
    }

    if (GetProfileFuelEditable() && fuelRateText_ != draft.fuelRateText)
    {
        fuelRateText_ = draft.fuelRateText;
        emit FuelRateTextChanged();
    }
    settings_.skipReposition = draft.skipReposition;
    settings_.callGpu = draft.callGpu;
    settings_.callCatering = draft.callCatering;
    settings_.callLavatory = draft.callLavatory;
    settings_.callWater = draft.callWater;
    settings_.callCleaning = draft.callCleaning;
    draft.useGlobal = true;

    emit SkipRepositionChanged();
    emit CallGpuChanged();
    emit CallCateringChanged();
    emit CallLavatoryChanged();
    emit CallWaterChanged();
    emit CallCleaningChanged();

    TouchProfileDraft();
}

void SettingsViewModel::applyProfileToAllProfiles()
{
    if (profileDrafts_.empty())
    {
        return;
    }

    const ProfileDraft source = SelectedDraft();
    for (ProfileDraft& draft : profileDrafts_)
    {
        draft = source;
    }

    TouchProfileDraft();
}

SettingsViewModel::ProfileDraft& SettingsViewModel::SelectedDraft()
{
    static ProfileDraft fallback;
    return profileDrafts_.empty() ? fallback : profileDrafts_[selectedProfileIndex_];
}

const SettingsViewModel::ProfileDraft& SettingsViewModel::SelectedDraft() const
{
    static const ProfileDraft fallback;
    return profileDrafts_.empty() ? fallback : profileDrafts_[selectedProfileIndex_];
}

void SettingsViewModel::TouchProfileDraft()
{
    SetSaveResult({}, false);

    emit ProfileDraftChanged();
    emit ValidationChanged();
}

bool SettingsViewModel::GetProfileUseGlobal() const
{
    return SelectedDraft().useGlobal;
}

void SettingsViewModel::SetProfileUseGlobal(const bool useGlobal)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal == useGlobal)
    {
        return;
    }

    ProfileDraft& draft = SelectedDraft();
    draft.useGlobal = useGlobal;
    if (!useGlobal)
    {
        draft.fuelRateText = fuelRateText_;
        draft.skipReposition = settings_.skipReposition;
        draft.callGpu = settings_.callGpu;
        draft.callCatering = settings_.callCatering;
        draft.callLavatory = settings_.callLavatory;
        draft.callWater = settings_.callWater;
        draft.callCleaning = settings_.callCleaning;
    }

    TouchProfileDraft();
}

QString SettingsViewModel::GetProfileFuelRateText() const
{
    return SelectedDraft().fuelRateText;
}

void SettingsViewModel::SetProfileFuelRateText(const QString& rate)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().fuelRateText == rate)
    {
        return;
    }

    SelectedDraft().fuelRateText = rate;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileSkipReposition() const
{
    return SelectedDraft().skipReposition;
}

void SettingsViewModel::SetProfileSkipReposition(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().skipReposition == enabled)
    {
        return;
    }

    SelectedDraft().skipReposition = enabled;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileCallGpu() const
{
    return SelectedDraft().callGpu;
}

void SettingsViewModel::SetProfileCallGpu(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().callGpu == enabled)
    {
        return;
    }

    SelectedDraft().callGpu = enabled;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileCallCatering() const
{
    return SelectedDraft().callCatering;
}

void SettingsViewModel::SetProfileCallCatering(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().callCatering == enabled)
    {
        return;
    }

    SelectedDraft().callCatering = enabled;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileCallLavatory() const
{
    return SelectedDraft().callLavatory;
}

void SettingsViewModel::SetProfileCallLavatory(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().callLavatory == enabled)
    {
        return;
    }

    SelectedDraft().callLavatory = enabled;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileCallWater() const
{
    return SelectedDraft().callWater;
}

void SettingsViewModel::SetProfileCallWater(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().callWater == enabled)
    {
        return;
    }

    SelectedDraft().callWater = enabled;

    TouchProfileDraft();
}

bool SettingsViewModel::GetProfileCallCleaning() const
{
    return SelectedDraft().callCleaning;
}

void SettingsViewModel::SetProfileCallCleaning(const bool enabled)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().callCleaning == enabled)
    {
        return;
    }

    SelectedDraft().callCleaning = enabled;

    TouchProfileDraft();
}

void SettingsViewModel::RefreshDetectedProfile()
{
    const std::string profileId = integratorService_->GetSnapshot().aircraftProfileId;
    detectedProfileIndex_ = -1;
    for (size_t i = 0; i < profileInfos_.size(); ++i)
    {
        if (profileInfos_[i].id == profileId)
        {
            detectedProfileIndex_ = static_cast<int>(i);
            break;
        }
    }
}

SettingsViewModel::Draft SettingsViewModel::Validate() const
{
    Draft result;

    const QString pilotText = simbriefPilotIdText_.trimmed();
    if (pilotText.isEmpty())
    {
        result.pilotId = 0;
    }
    else
    {
        bool pilotOk = false;
        result.pilotId = pilotText.toInt(&pilotOk);
        if (!pilotOk || result.pilotId <= 0)
        {
            result.error = tr("Enter a valid SimBrief Pilot ID.");

            return result;
        }
    }

    bool rateOk = false;
    result.fuelRateKgs = ParseFuelRate(fuelRateText_, &rateOk);
    if (!rateOk || result.fuelRateKgs <= 0.0)
    {
        result.error = tr("Enter a valid fuel rate.");

        return result;
    }

    for (size_t i = 0; i < profileInfos_.size(); ++i)
    {
        const ProfileDraft& draft = profileDrafts_[i];
        if (draft.useGlobal || profileInfos_[i].refuelBy != RefuelBy::Client)
        {
            continue;
        }

        bool profileRateOk = false;
        const double profileRate = ParseFuelRate(draft.fuelRateText, &profileRateOk);
        if (!profileRateOk || profileRate <= 0.0)
        {
            result.error = tr("Enter a valid fuel rate for %1.")
                .arg(QString::fromStdString(profileInfos_[i].shortCode));

            return result;
        }
        result.profileFuelRates.emplace(profileInfos_[i].id, profileRate);
    }

    result.valid = true;

    return result;
}

void SettingsViewModel::PersistImmediateSetting()
{
    if (!repository_->Save(settings_))
    {
        SetSaveResult(tr("Could not save settings."), true);

        return;
    }

    integratorService_->ApplySettings(settings_);
}

void SettingsViewModel::SetSaveResult(QString message, const bool error)
{
    if (saveMessage_ == message && saveError_ == error)
    {
        return;
    }

    saveMessage_ = std::move(message);
    saveError_ = error;

    emit SaveResultChanged();
}
