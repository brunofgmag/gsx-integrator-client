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
    AppSettings appliedSettings;
    int automationCalls = 0;
    int reloadCalls = 0;
    int applySettingsCalls = 0;

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

    [[nodiscard]] CommandResult ReloadSimbrief() override
    {
        ++reloadCalls;
        return CommandResult::Success();
    }

    void ApplySettings(const AppSettings& settings) override
    {
        appliedSettings = settings;
        ++applySettingsCalls;
    }

    void AddObserver(IntegratorServiceObserver* observer) override
    {
        if (observer != nullptr
            && std::find(observers_.begin(), observers_.end(), observer) == observers_.end())
        {
            observers_.push_back(observer);
        }
    }

    void RemoveObserver(IntegratorServiceObserver* observer) override
    {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    }

    void Notify()
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
