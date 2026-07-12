#ifndef GSX_INTEGRATOR_CLIENT_RUNTIMEINTEGRATORSERVICE_H
#define GSX_INTEGRATOR_CLIENT_RUNTIMEINTEGRATORSERVICE_H

#include <vector>
#include <QtCore/QObject>
#include "ports/IntegratorService.h"

class IntegratorRuntime;

class RuntimeIntegratorService final : public QObject, public IntegratorService
{
    Q_OBJECT

public:
    explicit RuntimeIntegratorService(IntegratorRuntime* runtime, QObject* parent = nullptr);

    [[nodiscard]] IntegratorSnapshot GetSnapshot() const override;
    [[nodiscard]] CommandResult SetAutomationEnabled(bool enabled) override;
    [[nodiscard]] CommandResult StartLoading() override;
    [[nodiscard]] CommandResult ReloadSimbrief() override;
    [[nodiscard]] CommandResult FixGsxProfile() override;
    void ApplySettings(const AppSettings& settings) override;

    void AddObserver(IntegratorServiceObserver* observer) override;
    void RemoveObserver(IntegratorServiceObserver* observer) override;

private:
    void NotifyObservers() const;

    IntegratorRuntime* runtime_;
    std::vector<IntegratorServiceObserver*> observers_;
};

#endif // GSX_INTEGRATOR_CLIENT_RUNTIMEINTEGRATORSERVICE_H
