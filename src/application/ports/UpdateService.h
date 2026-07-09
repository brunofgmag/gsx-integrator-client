#ifndef GSX_INTEGRATOR_CLIENT_UPDATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_UPDATESERVICE_H

#include <QtCore/QString>
#include "../model/UpdateInfo.h"

class UpdateServiceObserver
{
public:
    virtual ~UpdateServiceObserver() = default;
    virtual void OnCheckFinished(bool ok, bool updateAvailable, const UpdateInfo& info, const QString& error) = 0;
    virtual void OnCommbusCheckFinished(bool ok,
                                        const QString& installedVersion,
                                        const QString& latestVersion,
                                        const QString& releaseUrl) = 0;
    virtual void OnDownloadProgress(qint64 received, qint64 total) = 0;
    virtual void OnStageFinished(bool ok, const QString& error) = 0;
};

class UpdateService
{
public:
    virtual ~UpdateService() = default;

    virtual void CheckForUpdates() = 0;
    virtual void DownloadAndStage(const UpdateInfo& info) = 0;
    virtual void DiscardStaged() = 0;
    [[nodiscard]] virtual bool HasStagedUpdate() const = 0;
    virtual bool LaunchApplyHelper(bool relaunch) = 0;

    virtual void AddObserver(UpdateServiceObserver* observer) = 0;
    virtual void RemoveObserver(UpdateServiceObserver* observer) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_UPDATESERVICE_H
