#ifndef GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H
#define GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <functional>
#include "../application/model/AppSettings.h"

class SettingsRepository;
class IntegratorService;

class SettingsViewModel final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString simbriefPilotIdText READ GetSimbriefPilotIdText
        WRITE SetSimbriefPilotIdText NOTIFY SimbriefPilotIdTextChanged)
    Q_PROPERTY(QString fuelRateText READ GetFuelRateText WRITE SetFuelRateText NOTIFY FuelRateTextChanged)
    Q_PROPERTY(bool autoSelectGsxChoice READ GetAutoSelectGsxChoice
        WRITE SetAutoSelectGsxChoice NOTIFY AutoSelectGsxChoiceChanged)
    Q_PROPERTY(bool autoStartFlow READ GetAutoStartFlow WRITE SetAutoStartFlow NOTIFY AutoStartFlowChanged)
    Q_PROPERTY(bool autoStartLoading READ GetAutoStartLoading WRITE SetAutoStartLoading NOTIFY AutoStartLoadingChanged)
    Q_PROPERTY(bool skipReposition READ GetSkipReposition WRITE SetSkipReposition NOTIFY SkipRepositionChanged)
    Q_PROPERTY(int themeMode READ GetThemeMode WRITE SetThemeMode NOTIFY ThemeModeChanged)
    Q_PROPERTY(bool effectiveDark READ GetEffectiveDark NOTIFY EffectiveDarkChanged)
    Q_PROPERTY(QString language READ GetLanguage WRITE SetLanguage NOTIFY LanguageChanged)
    Q_PROPERTY(int updateMode READ GetUpdateMode WRITE SetUpdateMode NOTIFY UpdateModeChanged)
    Q_PROPERTY(bool closeToTray READ GetCloseToTray WRITE SetCloseToTray NOTIFY CloseToTrayChanged)
    Q_PROPERTY(bool minimizeToTray READ GetMinimizeToTray WRITE SetMinimizeToTray NOTIFY MinimizeToTrayChanged)
    Q_PROPERTY(bool trayTipShown READ GetTrayTipShown WRITE SetTrayTipShown NOTIFY TrayTipShownChanged)
    Q_PROPERTY(bool canSave READ CanSave NOTIFY ValidationChanged)
    Q_PROPERTY(QString validationMessage READ GetValidationMessage NOTIFY ValidationChanged)
    Q_PROPERTY(QString saveMessage READ GetSaveMessage NOTIFY SaveResultChanged)
    Q_PROPERTY(bool saveError READ HasSaveError NOTIFY SaveResultChanged)

public:
    enum ThemeMode { Light = 0, Dark = 1, System = 2 };

    Q_ENUM(ThemeMode)

    enum UpdateMode { Auto = 0, Notify = 1, Manual = 2 };

    Q_ENUM(UpdateMode)

    explicit SettingsViewModel(SettingsRepository* repository,
                               IntegratorService* integratorService,
                               QObject* parent = nullptr);

    [[nodiscard]] QString GetSimbriefPilotIdText() const;
    void SetSimbriefPilotIdText(const QString& pilotId);

    [[nodiscard]] QString GetFuelRateText() const;
    void SetFuelRateText(const QString& rate);

    [[nodiscard]] bool GetAutoSelectGsxChoice() const;
    void SetAutoSelectGsxChoice(bool enabled);

    [[nodiscard]] bool GetAutoStartFlow() const;
    void SetAutoStartFlow(bool enabled);

    [[nodiscard]] bool GetAutoStartLoading() const;
    void SetAutoStartLoading(bool enabled);

    [[nodiscard]] bool GetSkipReposition() const;
    void SetSkipReposition(bool enabled);

    [[nodiscard]] int GetThemeMode() const;
    void SetThemeMode(int mode);
    [[nodiscard]] bool GetEffectiveDark() const;

    void SetSystemDarkProvider(std::function<bool()> provider);
    void RefreshEffectiveTheme();

    [[nodiscard]] QString GetLanguage() const;
    void SetLanguage(const QString& language);

    [[nodiscard]] int GetUpdateMode() const;
    void SetUpdateMode(int mode);

    [[nodiscard]] bool GetCloseToTray() const;
    void SetCloseToTray(bool enabled);

    [[nodiscard]] bool GetMinimizeToTray() const;
    void SetMinimizeToTray(bool enabled);

    [[nodiscard]] bool GetTrayTipShown() const;
    void SetTrayTipShown(bool shown);

    void RetranslateUi();

    [[nodiscard]] bool CanSave() const;
    [[nodiscard]] QString GetValidationMessage() const;
    [[nodiscard]] QString GetSaveMessage() const;
    [[nodiscard]] bool HasSaveError() const;

    Q_INVOKABLE bool save();
    Q_INVOKABLE void clearSaveMessage();

signals:
    void SimbriefPilotIdTextChanged();
    void FuelRateTextChanged();
    void AutoSelectGsxChoiceChanged();
    void AutoStartFlowChanged();
    void AutoStartLoadingChanged();
    void SkipRepositionChanged();
    void ThemeModeChanged();
    void EffectiveDarkChanged();
    void LanguageChanged();
    void UpdateModeChanged();
    void CloseToTrayChanged();
    void MinimizeToTrayChanged();
    void TrayTipShownChanged();
    void ValidationChanged();
    void SaveResultChanged();

private:
    struct Draft
    {
        int pilotId = 0;
        double fuelRateKgs = 0.0;
        bool valid = false;
        QString error;
    };

    [[nodiscard]] Draft Validate() const;
    void PersistImmediateSetting();
    void SetSaveResult(QString message, bool error);

    SettingsRepository* repository_;
    IntegratorService* integratorService_;
    AppSettings settings_;

    std::function<bool()> systemDarkProvider_;

    QString simbriefPilotIdText_;
    QString fuelRateText_;
    QString saveMessage_;
    bool saveError_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H
