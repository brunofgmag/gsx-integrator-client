#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEUPDATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEUPDATESERVICE_H

#include <vector>
#include "../../src/application/ports/UpdateService.h"

class FakeUpdateService final : public UpdateService
{
public:
    void CheckForUpdates() override
    {
        ++checkCalls;
    }

    void DownloadAndStage(const UpdateInfo& info) override
    {
        ++downloadCalls;
        lastDownload = info;
    }

    void DiscardStaged() override
    {
        ++discardCalls;
        staged = false;
    }

    [[nodiscard]] bool HasStagedUpdate() const override
    {
        return staged;
    }

    bool LaunchApplyHelper(const bool relaunch) override
    {
        ++helperCalls;
        lastRelaunch = relaunch;
        return helperResult;
    }

    void AddObserver(UpdateServiceObserver* observer) override
    {
        observers.push_back(observer);
    }

    void RemoveObserver(UpdateServiceObserver* observer) override
    {
        std::erase(observers, observer);
    }

    void FireCheckFinished(const bool ok, const bool available, const UpdateInfo& info, const QString& error = {}) const
    {
        for (auto* observer : observers)
        {
            observer->OnCheckFinished(ok, available, info, error);
        }
    }

    void FireCommbusCheckFinished(const bool ok, const QString& installed, const QString& latest,
                                  const QString& url = {}) const
    {
        for (auto* observer : observers)
        {
            observer->OnCommbusCheckFinished(ok, installed, latest, url);
        }
    }

    void FireDownloadProgress(const qint64 received, const qint64 total) const
    {
        for (auto* observer : observers)
        {
            observer->OnDownloadProgress(received, total);
        }
    }

    void FireStageFinished(const bool ok, const QString& error = {})
    {
        staged = ok;
        for (auto* observer : observers)
        {
            observer->OnStageFinished(ok, error);
        }
    }

    std::vector<UpdateServiceObserver*> observers;
    UpdateInfo lastDownload;
    int checkCalls = 0;
    int downloadCalls = 0;
    int discardCalls = 0;
    int helperCalls = 0;
    bool lastRelaunch = false;
    bool helperResult = true;
    bool staged = false;
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEUPDATESERVICE_H
