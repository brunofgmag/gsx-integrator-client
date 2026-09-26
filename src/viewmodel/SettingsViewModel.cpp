#include "SettingsViewModel.h"

#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include "../application/ports/IntegratorService.h"
#include "../application/ports/SettingsRepository.h"
#include "../domain/support/Weight.h"

namespace
{
    constexpr std::array kGlobalFuelRateModes{FuelRateMode::Recommended, FuelRateMode::Manual};
    constexpr std::array kProfileFuelRateModes{FuelRateMode::Global, FuelRateMode::Recommended, FuelRateMode::Manual};

    template <std::size_t N>
    int FuelRateModeIndex(const std::array<FuelRateMode, N>& modes, const FuelRateMode mode)
    {
        const auto it = std::ranges::find(modes, mode);

        return it != modes.end() ? static_cast<int>(it - modes.begin()) : 0;
    }

    template <std::size_t N>
    bool IsFuelRateModeIndex(const std::array<FuelRateMode, N>& modes, const int index)
    {
        return index >= 0 && index < static_cast<int>(modes.size());
    }

    double ParseFuelRate(const QString& text, const bool lb, bool* ok)
    {
        const QString trimmed = text.trimmed();
        double value = QLocale().toDouble(trimmed, ok);
        if (!*ok)
        {
            value = QLocale::c().toDouble(trimmed, ok);
        }

        if (!*ok)
        {
            return 0.0;
        }

        return lb ? weight::LbToKg(value) : value;
    }

    std::optional<double> PositiveFuelRate(const QString& text, const bool lb)
    {
        bool ok = false;
        const double rate = ParseFuelRate(text, lb, &ok);
        if (ok && rate > 0.0)
        {
            return rate;
        }

        return std::nullopt;
    }

    QString FormatFuelRate(const double kgs, const bool lb)
    {
        const double shown = lb ? weight::KgToLb(kgs) : kgs;

        return QString::number(qRound64(shown));
    }

    QString ConvertRateText(const QString& text, const bool fromLb, const bool toLb)
    {
        if (fromLb == toLb)
        {
            return text;
        }

        bool ok = false;
        const double kgs = ParseFuelRate(text, fromLb, &ok);

        return ok ? FormatFuelRate(kgs, toLb) : text;
    }

    template <typename Dst, typename Src>
    void CopyServiceFields(Dst& dst, const Src& src)
    {
        dst.skipReposition = src.skipReposition;
        dst.callGpu = src.callGpu;
        dst.callGpuOnArrival = src.callGpuOnArrival;
        dst.callBoardingEarly = src.callBoardingEarly;
        dst.callCatering = src.callCatering;
        dst.callLavatory = src.callLavatory;
        dst.callWater = src.callWater;
        dst.callCleaning = src.callCleaning;
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
    displayIsLb_ = EffectiveIsLb();
    fuelRateText_ = FormatFuelRate(settings_.fuelRateKgs, displayIsLb_);
    fuelRateMode_ = settings_.fuelRateMode;
    integratorService_->ApplySettings(settings_);

    for (const AircraftProfileInfo& info : profileInfos_)
    {
        ProfileDraft draft;
        const auto it = settings_.profiles.find(info.id);
        if (it != settings_.profiles.end())
        {
            draft.useGlobal = it->second.useGlobal;
            draft.fuelRateMode = it->second.fuelRateMode;
            draft.fuelRateText = FormatFuelRate(it->second.fuelRateKgs, displayIsLb_);
            CopyServiceFields(draft, it->second);
        }
        else
        {
            draft.fuelRateText = fuelRateText_;
        }
        profileDrafts_.push_back(draft);
    }

    selectDetectedProfile();

    integratorService_->AddObserver(this);
}

SettingsViewModel::~SettingsViewModel()
{
    integratorService_->RemoveObserver(this);
}

void SettingsViewModel::OnIntegratorStateChanged()
{
    SyncDisplayUnit();
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

bool SettingsViewModel::GetStreamerMode() const
{
    return settings_.streamerMode;
}

void SettingsViewModel::SetStreamerMode(const bool enabled)
{
    SetPersisted(settings_.streamerMode, enabled, &SettingsViewModel::StreamerModeChanged);
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
    emit ProfileDraftChanged();
    emit ValidationChanged();
}

int SettingsViewModel::GetFuelRateModeIndex() const
{
    return FuelRateModeIndex(kGlobalFuelRateModes, fuelRateMode_);
}

void SettingsViewModel::SetFuelRateModeIndex(const int index)
{
    if (!IsFuelRateModeIndex(kGlobalFuelRateModes, index))
    {
        return;
    }

    const FuelRateMode mode = kGlobalFuelRateModes[static_cast<std::size_t>(index)];
    if (fuelRateMode_ == mode)
    {
        return;
    }

    fuelRateMode_ = mode;

    SetSaveResult({}, false);

    emit FuelRateModeChanged();
    emit ProfileDraftChanged();
    emit ValidationChanged();
}

bool SettingsViewModel::IsFuelRateEditable() const
{
    return fuelRateMode_ == FuelRateMode::Manual;
}

bool SettingsViewModel::GetAutoSelectGsxChoice() const
{
    return settings_.autoSelectGsxChoice;
}

void SettingsViewModel::SetAutoSelectGsxChoice(const bool enabled)
{
    SetPersisted(settings_.autoSelectGsxChoice, enabled, &SettingsViewModel::AutoSelectGsxChoiceChanged);
}

bool SettingsViewModel::GetAutoDeice() const
{
    return settings_.autoDeice;
}

void SettingsViewModel::SetAutoDeice(const bool enabled)
{
    SetPersisted(settings_.autoDeice, enabled, &SettingsViewModel::AutoDeiceChanged);
}

bool SettingsViewModel::GetUseAircraftStairs() const
{
    return settings_.useAircraftStairs;
}

void SettingsViewModel::SetUseAircraftStairs(const bool enabled)
{
    SetPersisted(settings_.useAircraftStairs, enabled, &SettingsViewModel::UseAircraftStairsChanged);
}

int SettingsViewModel::GetCrewBoarding() const
{
    return settings_.crewBoarding;
}

void SettingsViewModel::SetCrewBoarding(const int choice)
{
    SetPersisted(settings_.crewBoarding, choice, &SettingsViewModel::CrewBoardingChanged);
}

int SettingsViewModel::GetCrewDeboarding() const
{
    return settings_.crewDeboarding;
}

void SettingsViewModel::SetCrewDeboarding(const int choice)
{
    SetPersisted(settings_.crewDeboarding, choice, &SettingsViewModel::CrewDeboardingChanged);
}

bool SettingsViewModel::GetAutoStartFlow() const
{
    return settings_.autoStartFlow;
}

void SettingsViewModel::SetAutoStartFlow(const bool enabled)
{
    SetPersisted(settings_.autoStartFlow, enabled, &SettingsViewModel::AutoStartFlowChanged);
}

bool SettingsViewModel::GetAutoStartLoading() const
{
    return settings_.autoStartLoading;
}

void SettingsViewModel::SetAutoStartLoading(const bool enabled)
{
    SetPersisted(settings_.autoStartLoading, enabled, &SettingsViewModel::AutoStartLoadingChanged);
}

bool SettingsViewModel::GetSkipReposition() const
{
    return settings_.skipReposition;
}

void SettingsViewModel::SetSkipReposition(const bool enabled)
{
    if (SetPersisted(settings_.skipReposition, enabled, &SettingsViewModel::SkipRepositionChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallGpu() const
{
    return settings_.callGpu;
}

void SettingsViewModel::SetCallGpu(const bool enabled)
{
    if (SetPersisted(settings_.callGpu, enabled, &SettingsViewModel::CallGpuChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallGpuOnArrival() const
{
    return settings_.callGpuOnArrival;
}

void SettingsViewModel::SetCallGpuOnArrival(const bool enabled)
{
    if (SetPersisted(settings_.callGpuOnArrival, enabled, &SettingsViewModel::CallGpuOnArrivalChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallBoardingEarly() const
{
    return settings_.callBoardingEarly;
}

void SettingsViewModel::SetCallBoardingEarly(const bool enabled)
{
    if (SetPersisted(settings_.callBoardingEarly, enabled, &SettingsViewModel::CallBoardingEarlyChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallCatering() const
{
    return settings_.callCatering;
}

void SettingsViewModel::SetCallCatering(const bool enabled)
{
    if (SetPersisted(settings_.callCatering, enabled, &SettingsViewModel::CallCateringChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallLavatory() const
{
    return settings_.callLavatory;
}

void SettingsViewModel::SetCallLavatory(const bool enabled)
{
    if (SetPersisted(settings_.callLavatory, enabled, &SettingsViewModel::CallLavatoryChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallWater() const
{
    return settings_.callWater;
}

void SettingsViewModel::SetCallWater(const bool enabled)
{
    if (SetPersisted(settings_.callWater, enabled, &SettingsViewModel::CallWaterChanged))
    {
        emit ProfileDraftChanged();
    }
}

bool SettingsViewModel::GetCallCleaning() const
{
    return settings_.callCleaning;
}

void SettingsViewModel::SetCallCleaning(const bool enabled)
{
    if (SetPersisted(settings_.callCleaning, enabled, &SettingsViewModel::CallCleaningChanged))
    {
        emit ProfileDraftChanged();
    }
}

int SettingsViewModel::GetGsxPanelMode() const
{
    return settings_.gsxPanelMode;
}

void SettingsViewModel::SetGsxPanelMode(const int mode)
{
    SetPersisted(settings_.gsxPanelMode, mode, &SettingsViewModel::GsxPanelModeChanged);
}

int SettingsViewModel::GetThemeMode() const
{
    return settings_.themeMode;
}

void SettingsViewModel::SetThemeMode(const int mode)
{
    if (SetPersisted(settings_.themeMode, mode, &SettingsViewModel::ThemeModeChanged))
    {
        emit EffectiveDarkChanged();
    }
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
    SetPersisted(settings_.language, language.toStdString(), &SettingsViewModel::LanguageChanged);
}

QString SettingsViewModel::GetRenderer() const
{
    return QString::fromStdString(settings_.renderer);
}

void SettingsViewModel::SetRenderer(const QString& renderer)
{
    SetPersisted(settings_.renderer, renderer.toStdString(), &SettingsViewModel::RendererChanged);
}

QString SettingsViewModel::GetActiveRenderer() const
{
    return activeRenderer_;
}

void SettingsViewModel::SetActiveRenderer(const QString& renderer)
{
    if (activeRenderer_ == renderer)
    {
        return;
    }

    activeRenderer_ = renderer;

    emit ActiveRendererChanged();
}

int SettingsViewModel::GetUpdateMode() const
{
    return settings_.updateMode;
}

void SettingsViewModel::SetUpdateMode(const int mode)
{
    SetPersisted(settings_.updateMode, mode, &SettingsViewModel::UpdateModeChanged);
}

int SettingsViewModel::GetWeightUnitMode() const
{
    return settings_.weightUnitMode;
}

void SettingsViewModel::SetWeightUnitMode(const int mode)
{
    if (settings_.weightUnitMode == mode)
    {
        return;
    }

    settings_.weightUnitMode = mode;
    PersistImmediateSetting();

    emit WeightUnitModeChanged();

    SyncDisplayUnit();
}

bool SettingsViewModel::GetWeightIsLb() const
{
    return displayIsLb_;
}

QString SettingsViewModel::GetFuelRateUnitText() const
{
    return displayIsLb_ ? tr("lb/s") : tr("kg/s");
}

double SettingsViewModel::kgToLb(const double kg)
{
    return weight::KgToLb(kg);
}

bool SettingsViewModel::EffectiveIsLb() const
{
    switch (settings_.weightUnitMode)
    {
    case Kilograms:
        return false;
    case Pounds:
        return true;
    default:
        return integratorService_->GetSnapshot().autoWeightUnit == static_cast<int>(WeightUnit::Lb);
    }
}

void SettingsViewModel::SyncDisplayUnit()
{
    const bool nowLb = EffectiveIsLb();
    if (nowLb == displayIsLb_)
    {
        return;
    }

    RescaleFuelRateTexts(displayIsLb_, nowLb);
    displayIsLb_ = nowLb;

    emit WeightUnitDisplayChanged();
}

void SettingsViewModel::RescaleFuelRateTexts(const bool fromLb, const bool toLb)
{
    fuelRateText_ = ConvertRateText(fuelRateText_, fromLb, toLb);
    emit FuelRateTextChanged();

    for (ProfileDraft& draft : profileDrafts_)
    {
        draft.fuelRateText = ConvertRateText(draft.fuelRateText, fromLb, toLb);
    }
    emit ProfileDraftChanged();
}

bool SettingsViewModel::GetCloseToTray() const
{
    return settings_.closeToTray;
}

void SettingsViewModel::SetCloseToTray(const bool enabled)
{
    SetPersisted(settings_.closeToTray, enabled, &SettingsViewModel::CloseToTrayChanged);
}

bool SettingsViewModel::GetMinimizeToTray() const
{
    return settings_.minimizeToTray;
}

void SettingsViewModel::SetMinimizeToTray(const bool enabled)
{
    SetPersisted(settings_.minimizeToTray, enabled, &SettingsViewModel::MinimizeToTrayChanged);
}

bool SettingsViewModel::GetTrayTipShown() const
{
    return settings_.trayTipShown;
}

void SettingsViewModel::SetTrayTipShown(const bool shown)
{
    SetPersisted(settings_.trayTipShown, shown, &SettingsViewModel::TrayTipShownChanged);
}

bool SettingsViewModel::GetCommbusManaged() const
{
    return settings_.commbusManaged;
}

void SettingsViewModel::SetCommbusManaged(const bool managed)
{
    SetPersisted(settings_.commbusManaged, managed, &SettingsViewModel::CommbusManagedChanged);
}

bool SettingsViewModel::GetLoggingEnabled() const
{
    return settings_.loggingEnabled;
}

void SettingsViewModel::SetLoggingEnabled(const bool enabled)
{
    SetPersisted(settings_.loggingEnabled, enabled, &SettingsViewModel::LoggingEnabledChanged);
}

bool SettingsViewModel::GetLoggingActive() const
{
    return loggingActive_;
}

void SettingsViewModel::SetLoggingActive(const bool active)
{
    if (loggingActive_ == active)
    {
        return;
    }

    loggingActive_ = active;

    emit LoggingActiveChanged();
}

bool SettingsViewModel::AreDebugToolsAvailable()
{
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
}

QString SettingsViewModel::GetLogLocation() const
{
    return logLocation_;
}

void SettingsViewModel::SetLogLocation(const QString& location)
{
    const QString native = QDir::toNativeSeparators(location);
    if (logLocation_ == native)
    {
        return;
    }

    logLocation_ = native;

    emit LogLocationChanged();
}

void SettingsViewModel::RetranslateUi()
{
    emit ValidationChanged();
    emit ProfileSelectionChanged();
    emit WeightUnitDisplayChanged();
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
    settings_.fuelRateMode = fuelRateMode_;
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
        profile.fuelRateMode = draft.fuelRateMode;
        const auto rate = profileFuelRates.find(profileInfos_[i].id);
        profile.fuelRateKgs = rate != profileFuelRates.end() ? rate->second : settings_.fuelRateKgs;
        CopyServiceFields(profile, draft);
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

    if (GetProfileFuelEditable())
    {
        if (fuelRateText_ != draft.fuelRateText)
        {
            fuelRateText_ = draft.fuelRateText;
            emit FuelRateTextChanged();
        }
        if (draft.fuelRateMode != FuelRateMode::Global && fuelRateMode_ != draft.fuelRateMode)
        {
            fuelRateMode_ = draft.fuelRateMode;
            emit FuelRateModeChanged();
        }
    }
    CopyServiceFields(settings_, draft);
    draft.useGlobal = true;

    emit SkipRepositionChanged();
    emit CallGpuChanged();
    emit CallGpuOnArrivalChanged();
    emit CallBoardingEarlyChanged();
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

void SettingsViewModel::SetProfileToggle(bool ProfileDraft::* member, const bool value)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || SelectedDraft().*member == value)
    {
        return;
    }

    SelectedDraft().*member = value;

    TouchProfileDraft();
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
        draft.fuelRateMode = fuelRateMode_;
        draft.fuelRateText = fuelRateText_;
        CopyServiceFields(draft, settings_);
    }

    TouchProfileDraft();
}

QString SettingsViewModel::GetProfileFuelRateText() const
{
    return SelectedDraft().useGlobal ? fuelRateText_ : SelectedDraft().fuelRateText;
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

int SettingsViewModel::GetProfileFuelRateModeIndex() const
{
    const FuelRateMode mode = SelectedDraft().useGlobal ? fuelRateMode_ : SelectedDraft().fuelRateMode;

    return FuelRateModeIndex(kProfileFuelRateModes, mode);
}

void SettingsViewModel::SetProfileFuelRateModeIndex(const int index)
{
    if (profileDrafts_.empty() || SelectedDraft().useGlobal || !IsFuelRateModeIndex(kProfileFuelRateModes, index))
    {
        return;
    }

    const FuelRateMode mode = kProfileFuelRateModes[static_cast<std::size_t>(index)];
    if (SelectedDraft().fuelRateMode == mode)
    {
        return;
    }

    SelectedDraft().fuelRateMode = mode;

    TouchProfileDraft();
}

bool SettingsViewModel::IsProfileFuelRateEditable() const
{
    return GetProfileFuelEditable()
        && !SelectedDraft().useGlobal
        && SelectedDraft().fuelRateMode == FuelRateMode::Manual;
}

QString SettingsViewModel::GetProfileRecommendedFuelRateText() const
{
    if (profileInfos_.empty())
    {
        return {};
    }

    return FormatFuelRate(profileInfos_[selectedProfileIndex_].recommendedFuelRateKgs, displayIsLb_);
}

bool SettingsViewModel::GetProfileSkipReposition() const
{
    return SelectedDraft().useGlobal ? settings_.skipReposition : SelectedDraft().skipReposition;
}

void SettingsViewModel::SetProfileSkipReposition(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::skipReposition, enabled);
}

bool SettingsViewModel::GetProfileCallGpu() const
{
    return SelectedDraft().useGlobal ? settings_.callGpu : SelectedDraft().callGpu;
}

void SettingsViewModel::SetProfileCallGpu(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callGpu, enabled);
}

bool SettingsViewModel::GetProfileCallGpuOnArrival() const
{
    return SelectedDraft().useGlobal ? settings_.callGpuOnArrival : SelectedDraft().callGpuOnArrival;
}

void SettingsViewModel::SetProfileCallGpuOnArrival(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callGpuOnArrival, enabled);
}

bool SettingsViewModel::GetProfileCallBoardingEarly() const
{
    return SelectedDraft().useGlobal ? settings_.callBoardingEarly : SelectedDraft().callBoardingEarly;
}

void SettingsViewModel::SetProfileCallBoardingEarly(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callBoardingEarly, enabled);
}

bool SettingsViewModel::GetProfileCallCatering() const
{
    return SelectedDraft().useGlobal ? settings_.callCatering : SelectedDraft().callCatering;
}

void SettingsViewModel::SetProfileCallCatering(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callCatering, enabled);
}

bool SettingsViewModel::GetProfileCallLavatory() const
{
    return SelectedDraft().useGlobal ? settings_.callLavatory : SelectedDraft().callLavatory;
}

void SettingsViewModel::SetProfileCallLavatory(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callLavatory, enabled);
}

bool SettingsViewModel::GetProfileCallWater() const
{
    return SelectedDraft().useGlobal ? settings_.callWater : SelectedDraft().callWater;
}

void SettingsViewModel::SetProfileCallWater(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callWater, enabled);
}

bool SettingsViewModel::GetProfileCallCleaning() const
{
    return SelectedDraft().useGlobal ? settings_.callCleaning : SelectedDraft().callCleaning;
}

void SettingsViewModel::SetProfileCallCleaning(const bool enabled)
{
    SetProfileToggle(&ProfileDraft::callCleaning, enabled);
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

    const std::optional<double> globalRate = PositiveFuelRate(fuelRateText_, displayIsLb_);
    if (!globalRate && fuelRateMode_ == FuelRateMode::Manual)
    {
        result.error = tr("Enter a valid fuel rate.");

        return result;
    }
    result.fuelRateKgs = globalRate.value_or(settings_.fuelRateKgs);

    for (size_t i = 0; i < profileInfos_.size(); ++i)
    {
        const ProfileDraft& draft = profileDrafts_[i];
        if (draft.useGlobal || profileInfos_[i].refuelBy != RefuelBy::Client)
        {
            continue;
        }

        const std::optional<double> profileRate = PositiveFuelRate(draft.fuelRateText, displayIsLb_);
        if (!profileRate && draft.fuelRateMode == FuelRateMode::Manual)
        {
            result.error = tr("Enter a valid fuel rate for %1.")
                .arg(QString::fromStdString(profileInfos_[i].shortCode));

            return result;
        }
        if (profileRate)
        {
            result.profileFuelRates.emplace(profileInfos_[i].id, *profileRate);
        }
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
