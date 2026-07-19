#ifndef GSX_INTEGRATOR_CLIENT_INTEGRATORSERVICE_H
#define GSX_INTEGRATOR_CLIENT_INTEGRATORSERVICE_H

#include "../model/CommandResult.h"
#include "../model/AppSettings.h"
#include "../model/IntegratorSnapshot.h"

class IntegratorServiceObserver
{
public:
    virtual ~IntegratorServiceObserver() = default;
    virtual void OnIntegratorStateChanged() = 0;
};

class IntegratorService
{
public:
    virtual ~IntegratorService() = default;

    [[nodiscard]] virtual IntegratorSnapshot GetSnapshot() const = 0;

    [[nodiscard]] virtual CommandResult SetAutomationEnabled(bool enabled) = 0;
    [[nodiscard]] virtual CommandResult StartLoading() = 0;
    [[nodiscard]] virtual CommandResult RestartFlow() = 0;
    [[nodiscard]] virtual CommandResult ReloadSimbrief() = 0;
    [[nodiscard]] virtual CommandResult FixGsxProfile() = 0;

    virtual void ApplySettings(const AppSettings& settings) = 0;

#ifndef NDEBUG
    virtual void DebugSkipPhase(int) {}
#endif

    virtual void AddObserver(IntegratorServiceObserver* observer) = 0;
    virtual void RemoveObserver(IntegratorServiceObserver* observer) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INTEGRATORSERVICE_H
