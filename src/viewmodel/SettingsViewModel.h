#ifndef GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H
#define GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <functional>
#include <map>
#include <vector>
#include "../application/model/AppSettings.h"
#include "../application/model/AircraftProfile.h"
#include "../application/ports/IntegratorService.h"

class SettingsRepository;

class SettingsViewModel final : public QObject, public IntegratorServiceObserver
{
    Q_OBJECT

    Q_PROPERTY(QString simbriefPilotIdText READ GetSimbriefPilotIdText
        WRITE SetSimbriefPilotIdText NOTIFY SimbriefPilotIdTextChanged)
    Q_PROPERTY(bool streamerMode READ GetStreamerMode WRITE SetStreamerMode NOTIFY StreamerModeChanged)
    Q_PROPERTY(QString fuelRateText READ GetFuelRateText WRITE SetFuelRateText NOTIFY FuelRateTextChanged)
    Q_PROPERTY(bool weightIsLb READ GetWeightIsLb NOTIFY WeightUnitDisplayChanged)
    Q_PROPERTY(QString fuelRateUnitText READ GetFuelRateUnitText NOTIFY WeightUnitDisplayChanged)
    Q_PROPERTY(bool autoSelectGsxChoice READ GetAutoSelectGsxChoice
        WRITE SetAutoSelectGsxChoice NOTIFY AutoSelectGsxChoiceChanged)
    Q_PROPERTY(bool autoDeice READ GetAutoDeice WRITE SetAutoDeice NOTIFY AutoDeiceChanged)
    Q_PROPERTY(int crewBoarding READ GetCrewBoarding WRITE SetCrewBoarding NOTIFY CrewBoardingChanged)
    Q_PROPERTY(bool autoStartFlow READ GetAutoStartFlow WRITE SetAutoStartFlow NOTIFY AutoStartFlowChanged)
    Q_PROPERTY(bool autoStartLoading READ GetAutoStartLoading WRITE SetAutoStartLoading NOTIFY AutoStartLoadingChanged)
    Q_PROPERTY(bool skipReposition READ GetSkipReposition WRITE SetSkipReposition NOTIFY SkipRepositionChanged)
    Q_PROPERTY(bool callGpu READ GetCallGpu WRITE SetCallGpu NOTIFY CallGpuChanged)
    Q_PROPERTY(bool callGpuOnArrival READ GetCallGpuOnArrival WRITE SetCallGpuOnArrival NOTIFY CallGpuOnArrivalChanged)
    Q_PROPERTY(bool callCatering READ GetCallCatering WRITE SetCallCatering NOTIFY CallCateringChanged)
    Q_PROPERTY(bool callLavatory READ GetCallLavatory WRITE SetCallLavatory NOTIFY CallLavatoryChanged)
    Q_PROPERTY(bool callWater READ GetCallWater WRITE SetCallWater NOTIFY CallWaterChanged)
    Q_PROPERTY(bool callCleaning READ GetCallCleaning WRITE SetCallCleaning NOTIFY CallCleaningChanged)
    Q_PROPERTY(bool openGsxOnRequests READ GetOpenGsxOnRequests
        WRITE SetOpenGsxOnRequests NOTIFY OpenGsxOnRequestsChanged)
    Q_PROPERTY(int themeMode READ GetThemeMode WRITE SetThemeMode NOTIFY ThemeModeChanged)
    Q_PROPERTY(bool effectiveDark READ GetEffectiveDark NOTIFY EffectiveDarkChanged)
    Q_PROPERTY(QString language READ GetLanguage WRITE SetLanguage NOTIFY LanguageChanged)
    Q_PROPERTY(int updateMode READ GetUpdateMode WRITE SetUpdateMode NOTIFY UpdateModeChanged)
    Q_PROPERTY(int weightUnitMode READ GetWeightUnitMode WRITE SetWeightUnitMode NOTIFY WeightUnitModeChanged)
    Q_PROPERTY(bool closeToTray READ GetCloseToTray WRITE SetCloseToTray NOTIFY CloseToTrayChanged)
    Q_PROPERTY(bool minimizeToTray READ GetMinimizeToTray WRITE SetMinimizeToTray NOTIFY MinimizeToTrayChanged)
    Q_PROPERTY(bool trayTipShown READ GetTrayTipShown WRITE SetTrayTipShown NOTIFY TrayTipShownChanged)
    Q_PROPERTY(bool canSave READ CanSave NOTIFY ValidationChanged)
    Q_PROPERTY(QString validationMessage READ GetValidationMessage NOTIFY ValidationChanged)
    Q_PROPERTY(QString saveMessage READ GetSaveMessage NOTIFY SaveResultChanged)
    Q_PROPERTY(bool saveError READ HasSaveError NOTIFY SaveResultChanged)
    Q_PROPERTY(QVariantList profileModel READ GetProfileModel NOTIFY ProfileModelChanged)
    Q_PROPERTY(int selectedProfileIndex READ GetSelectedProfileIndex
        WRITE SetSelectedProfileIndex NOTIFY ProfileSelectionChanged)
    Q_PROPERTY(int detectedProfileIndex READ GetDetectedProfileIndex NOTIFY ProfileSelectionChanged)
    Q_PROPERTY(bool profileFuelEditable READ GetProfileFuelEditable NOTIFY ProfileSelectionChanged)
    Q_PROPERTY(QString profileFuelBadge READ GetProfileFuelBadge NOTIFY ProfileSelectionChanged)
    Q_PROPERTY(bool profileUseGlobal READ GetProfileUseGlobal
        WRITE SetProfileUseGlobal NOTIFY ProfileDraftChanged)
    Q_PROPERTY(QString profileFuelRateText READ GetProfileFuelRateText
        WRITE SetProfileFuelRateText NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileSkipReposition READ GetProfileSkipReposition
        WRITE SetProfileSkipReposition NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallGpu READ GetProfileCallGpu
        WRITE SetProfileCallGpu NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallGpuOnArrival READ GetProfileCallGpuOnArrival
        WRITE SetProfileCallGpuOnArrival NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallCatering READ GetProfileCallCatering
        WRITE SetProfileCallCatering NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallLavatory READ GetProfileCallLavatory
        WRITE SetProfileCallLavatory NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallWater READ GetProfileCallWater
        WRITE SetProfileCallWater NOTIFY ProfileDraftChanged)
    Q_PROPERTY(bool profileCallCleaning READ GetProfileCallCleaning
        WRITE SetProfileCallCleaning NOTIFY ProfileDraftChanged)

public:
    enum ThemeMode { Light = 0, Dark = 1, System = 2 };

    Q_ENUM(ThemeMode)

    enum UpdateMode { Auto = 0, Notify = 1, Manual = 2 };

    Q_ENUM(UpdateMode)

    enum WeightUnitMode { AutoUnit = 0, Kilograms = 1, Pounds = 2 };

    Q_ENUM(WeightUnitMode)

    explicit SettingsViewModel(SettingsRepository* repository,
                               IntegratorService* integratorService,
                               std::vector<AircraftProfileInfo> profileInfos = {},
                               QObject* parent = nullptr);
    ~SettingsViewModel() override;

    void OnIntegratorStateChanged() override;

    [[nodiscard]] QString GetSimbriefPilotIdText() const;
    void SetSimbriefPilotIdText(const QString& pilotId);

    [[nodiscard]] bool GetStreamerMode() const;
    void SetStreamerMode(bool enabled);

    [[nodiscard]] QString GetFuelRateText() const;
    void SetFuelRateText(const QString& rate);

    [[nodiscard]] bool GetAutoSelectGsxChoice() const;
    void SetAutoSelectGsxChoice(bool enabled);

    [[nodiscard]] bool GetAutoDeice() const;
    void SetAutoDeice(bool enabled);

    [[nodiscard]] int GetCrewBoarding() const;
    void SetCrewBoarding(int choice);

    [[nodiscard]] bool GetAutoStartFlow() const;
    void SetAutoStartFlow(bool enabled);

    [[nodiscard]] bool GetAutoStartLoading() const;
    void SetAutoStartLoading(bool enabled);

    [[nodiscard]] bool GetSkipReposition() const;
    void SetSkipReposition(bool enabled);

    [[nodiscard]] bool GetCallGpu() const;
    void SetCallGpu(bool enabled);

    [[nodiscard]] bool GetCallGpuOnArrival() const;
    void SetCallGpuOnArrival(bool enabled);

    [[nodiscard]] bool GetCallCatering() const;
    void SetCallCatering(bool enabled);

    [[nodiscard]] bool GetCallLavatory() const;
    void SetCallLavatory(bool enabled);

    [[nodiscard]] bool GetCallWater() const;
    void SetCallWater(bool enabled);

    [[nodiscard]] bool GetCallCleaning() const;
    void SetCallCleaning(bool enabled);

    [[nodiscard]] bool GetOpenGsxOnRequests() const;
    void SetOpenGsxOnRequests(bool enabled);

    [[nodiscard]] int GetThemeMode() const;
    void SetThemeMode(int mode);
    [[nodiscard]] bool GetEffectiveDark() const;

    void SetSystemDarkProvider(std::function<bool()> provider);
    void RefreshEffectiveTheme();

    [[nodiscard]] QString GetLanguage() const;
    void SetLanguage(const QString& language);

    [[nodiscard]] int GetUpdateMode() const;
    void SetUpdateMode(int mode);

    [[nodiscard]] int GetWeightUnitMode() const;
    void SetWeightUnitMode(int mode);
    [[nodiscard]] bool GetWeightIsLb() const;
    [[nodiscard]] QString GetFuelRateUnitText() const;
    [[nodiscard]] Q_INVOKABLE static double kgToLb(double kg);

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

    [[nodiscard]] QVariantList GetProfileModel() const;
    [[nodiscard]] int GetSelectedProfileIndex() const;
    void SetSelectedProfileIndex(int index);
    [[nodiscard]] int GetDetectedProfileIndex() const;
    [[nodiscard]] bool GetProfileFuelEditable() const;
    [[nodiscard]] QString GetProfileFuelBadge() const;
    Q_INVOKABLE void selectDetectedProfile();
    Q_INVOKABLE void setProfileAsGlobalDefault();
    Q_INVOKABLE void applyProfileToAllProfiles();

    [[nodiscard]] bool GetProfileUseGlobal() const;
    void SetProfileUseGlobal(bool useGlobal);

    [[nodiscard]] QString GetProfileFuelRateText() const;
    void SetProfileFuelRateText(const QString& rate);

    [[nodiscard]] bool GetProfileSkipReposition() const;
    void SetProfileSkipReposition(bool enabled);

    [[nodiscard]] bool GetProfileCallGpu() const;
    void SetProfileCallGpu(bool enabled);

    [[nodiscard]] bool GetProfileCallGpuOnArrival() const;
    void SetProfileCallGpuOnArrival(bool enabled);

    [[nodiscard]] bool GetProfileCallCatering() const;
    void SetProfileCallCatering(bool enabled);

    [[nodiscard]] bool GetProfileCallLavatory() const;
    void SetProfileCallLavatory(bool enabled);

    [[nodiscard]] bool GetProfileCallWater() const;
    void SetProfileCallWater(bool enabled);

    [[nodiscard]] bool GetProfileCallCleaning() const;
    void SetProfileCallCleaning(bool enabled);

signals:
    void SimbriefPilotIdTextChanged();
    void StreamerModeChanged();
    void FuelRateTextChanged();
    void AutoSelectGsxChoiceChanged();
    void AutoDeiceChanged();
    void CrewBoardingChanged();
    void AutoStartFlowChanged();
    void AutoStartLoadingChanged();
    void SkipRepositionChanged();
    void CallGpuChanged();
    void CallGpuOnArrivalChanged();
    void CallCateringChanged();
    void CallLavatoryChanged();
    void CallWaterChanged();
    void CallCleaningChanged();
    void OpenGsxOnRequestsChanged();
    void ThemeModeChanged();
    void EffectiveDarkChanged();
    void LanguageChanged();
    void UpdateModeChanged();
    void WeightUnitModeChanged();
    void WeightUnitDisplayChanged();
    void CloseToTrayChanged();
    void MinimizeToTrayChanged();
    void TrayTipShownChanged();
    void ValidationChanged();
    void SaveResultChanged();
    void ProfileModelChanged();
    void ProfileSelectionChanged();
    void ProfileDraftChanged();

private:
    struct Draft
    {
        int pilotId = 0;
        double fuelRateKgs = 0.0;
        std::map<std::string, double> profileFuelRates;
        bool valid = false;
        QString error;
    };

    struct ProfileDraft
    {
        bool useGlobal = true;
        QString fuelRateText;
        bool skipReposition = false;
        bool callGpu = false;
        bool callGpuOnArrival = false;
        bool callCatering = false;
        bool callLavatory = false;
        bool callWater = false;
        bool callCleaning = false;
    };

    [[nodiscard]] Draft Validate() const;
    [[nodiscard]] bool EffectiveIsLb() const;
    void SyncDisplayUnit();
    void RescaleFuelRateTexts(bool fromLb, bool toLb);
    void PersistImmediateSetting();
    void SetSaveResult(QString message, bool error);
    void RefreshDetectedProfile();
    [[nodiscard]] ProfileDraft& SelectedDraft();
    [[nodiscard]] const ProfileDraft& SelectedDraft() const;
    void TouchProfileDraft();
    void SetProfileToggle(bool ProfileDraft::* member, bool value);

    template <typename T>
    bool SetPersisted(T& field, const T& value, void (SettingsViewModel::*signal)())
    {
        if (field == value)
        {
            return false;
        }

        field = value;

        PersistImmediateSetting();

        emit (this->*signal)();

        return true;
    }

    SettingsRepository* repository_;
    IntegratorService* integratorService_;
    AppSettings settings_;

    std::function<bool()> systemDarkProvider_;

    QString simbriefPilotIdText_;
    QString fuelRateText_;
    bool displayIsLb_ = false;
    QString saveMessage_;
    bool saveError_ = false;

    std::vector<AircraftProfileInfo> profileInfos_;
    int selectedProfileIndex_ = 0;
    int detectedProfileIndex_ = -1;
    std::vector<ProfileDraft> profileDrafts_;
};

#endif // GSX_INTEGRATOR_CLIENT_SETTINGSVIEWMODEL_H
