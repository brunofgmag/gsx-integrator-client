#ifndef GSX_INTEGRATOR_CLIENT_UPDATEVIEWMODEL_H
#define GSX_INTEGRATOR_CLIENT_UPDATEVIEWMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include "../application/model/UpdateInfo.h"
#include "../application/ports/UpdateService.h"

class UpdateViewModel final : public QObject, public UpdateServiceObserver
{
    Q_OBJECT

    Q_PROPERTY(int state READ GetState NOTIFY StateChanged)
    Q_PROPERTY(bool updateAvailable READ IsUpdateAvailable NOTIFY StateChanged)
    Q_PROPERTY(bool readyToRestart READ IsReadyToRestart NOTIFY StateChanged)
    Q_PROPERTY(QString currentVersion READ GetCurrentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ GetLatestVersion NOTIFY StateChanged)
    Q_PROPERTY(double progress READ GetProgress NOTIFY ProgressChanged)
    Q_PROPERTY(QString errorMessage READ GetErrorMessage NOTIFY StateChanged)
    Q_PROPERTY(QString releaseUrl READ GetReleaseUrl NOTIFY StateChanged)
    Q_PROPERTY(bool checksEnabled READ AreChecksEnabled CONSTANT)
    Q_PROPERTY(bool commbusUpdateAvailable READ IsCommbusUpdateAvailable NOTIFY CommbusChanged)
    Q_PROPERTY(QString commbusInstalledVersion READ GetCommbusInstalledVersion NOTIFY CommbusChanged)
    Q_PROPERTY(QString commbusLatestVersion READ GetCommbusLatestVersion NOTIFY CommbusChanged)
    Q_PROPERTY(QString commbusReleaseUrl READ GetCommbusReleaseUrl NOTIFY CommbusChanged)

public:
    enum State { Idle = 0, Checking, UpToDate, UpdateAvailable, Downloading, ReadyToRestart, Error };
    Q_ENUM(State)

    enum Mode { Auto = 0, Notify = 1, Manual = 2 };
    Q_ENUM(Mode)

    UpdateViewModel(UpdateService* service,
                    QString currentVersion,
                    int initialMode,
                    bool updatesEnabled,
                    QObject* parent = nullptr);
    ~UpdateViewModel() override;

    [[nodiscard]] int GetState() const;
    [[nodiscard]] bool IsUpdateAvailable() const;
    [[nodiscard]] bool IsReadyToRestart() const;
    [[nodiscard]] QString GetCurrentVersion() const;
    [[nodiscard]] QString GetLatestVersion() const;
    [[nodiscard]] double GetProgress() const;
    [[nodiscard]] QString GetErrorMessage() const;
    [[nodiscard]] QString GetReleaseUrl() const;
    [[nodiscard]] bool AreChecksEnabled() const;
    [[nodiscard]] bool IsCommbusUpdateAvailable() const;
    [[nodiscard]] QString GetCommbusInstalledVersion() const;
    [[nodiscard]] QString GetCommbusLatestVersion() const;
    [[nodiscard]] QString GetCommbusReleaseUrl() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadAndInstall();
    Q_INVOKABLE void restartNow();

    void SetMode(int mode);
    [[nodiscard]] bool ShouldApplyOnExit() const;

    void OnCheckFinished(bool ok, bool updateAvailable,
                         const UpdateInfo& info, const QString& error) override;
    void OnCommbusCheckFinished(bool ok, const QString& installedVersion,
                                const QString& latestVersion,
                                const QString& releaseUrl) override;
    void OnDownloadProgress(qint64 received, qint64 total) override;
    void OnStageFinished(bool ok, const QString& error) override;

signals:
    void StateChanged();
    void ProgressChanged();
    void CommbusChanged();

private:
    void StartBackgroundCheck();
    void SetState(State state);

    UpdateService* service_;
    QString currentVersion_;
    int mode_;
    bool updatesEnabled_;

    State state_ = Idle;
    bool updateKnown_ = false;
    bool explicitCheck_ = false;
    bool restartWhenStaged_ = false;
    UpdateInfo latest_;
    double progress_ = 0.0;
    QString errorMessage_;

    bool commbusUpdateAvailable_ = false;
    QString commbusInstalledVersion_;
    QString commbusLatestVersion_;
    QString commbusReleaseUrl_;

    QTimer startupTimer_;
    QTimer periodicTimer_;
};

#endif // GSX_INTEGRATOR_CLIENT_UPDATEVIEWMODEL_H
