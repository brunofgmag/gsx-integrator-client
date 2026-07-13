#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEINTEGRATORSERVICE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEINTEGRATORSERVICE_H

#include <algorithm>
#include <vector>
#include "../../src/application/ports/IntegratorService.h"

class FakeIntegratorService final : public IntegratorService
{
public:
    IntegratorSnapshot snapshot;
    CommandResult automationResult = CommandResult::Success();
    CommandResult startLoadingResult = CommandResult::Success();
    CommandResult restartFlowResult = CommandResult::Success();
    CommandResult fixGsxProfileResult = CommandResult::Success();
    AppSettings appliedSettings;
    int automationCalls = 0;
    int startLoadingCalls = 0;
    int restartFlowCalls = 0;
    int reloadCalls = 0;
    int applySettingsCalls = 0;
    int fixGsxProfileCalls = 0;

    [[nodiscard]] IntegratorSnapshot GetSnapshot() const override
    {
        return snapshot;
    }

    [[nodiscard]] CommandResult SetAutomationEnabled(const bool enabled) override
    {
        ++automationCalls;
        if (automationResult.succeeded)
        {
            snapshot.automationEnabled = enabled;
            Notify();
        }
        return automationResult;
    }

    [[nodiscard]] CommandResult StartLoading() override
    {
        ++startLoadingCalls;
        if (startLoadingResult.succeeded)
        {
            snapshot.canStartLoading = false;
            Notify();
        }

        return startLoadingResult;
    }

    [[nodiscard]] CommandResult RestartFlow() override
    {
        ++restartFlowCalls;
        if (restartFlowResult.succeeded)
        {
            Notify();
        }

        return restartFlowResult;
    }

    [[nodiscard]] CommandResult ReloadSimbrief() override
    {
        ++reloadCalls;
        return CommandResult::Success();
    }

    [[nodiscard]] CommandResult FixGsxProfile() override
    {
        ++fixGsxProfileCalls;
        if (fixGsxProfileResult.succeeded)
        {
            snapshot.gsxProfileConflict = false;
            snapshot.gsxProfileFixable = false;
            Notify();
        }

        return fixGsxProfileResult;
    }

    void ApplySettings(const AppSettings& settings) override
    {
        appliedSettings = settings;
        ++applySettingsCalls;
    }

    void AddObserver(IntegratorServiceObserver* observer) override
    {
        if (observer != nullptr
            && std::ranges::find(observers_, observer) == observers_.end())
        {
            observers_.push_back(observer);
        }
    }

    void RemoveObserver(IntegratorServiceObserver* observer) override
    {
        std::erase(observers_, observer);
    }

    void Notify() const
    {
        const auto observers = observers_;
        for (IntegratorServiceObserver* observer : observers)
        {
            observer->OnIntegratorStateChanged();
        }
    }

private:
    std::vector<IntegratorServiceObserver*> observers_;
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEINTEGRATORSERVICE_H
