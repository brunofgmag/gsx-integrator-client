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
    QObject* parent)
    : QObject(parent),
      repository_(repository),
      integratorService_(integratorService),
      settings_(repository_->Load())
{
    simbriefPilotIdText_ = settings_.simbriefPilotId > 0
                               ? QString::number(settings_.simbriefPilotId)
                               : QString();
    fuelRateText_ = FormatFuelRate(settings_.fuelRateKgs);
    integratorService_->ApplySettings(settings_);
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
