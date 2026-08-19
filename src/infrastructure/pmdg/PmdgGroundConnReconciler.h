#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNRECONCILER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNRECONCILER_H

#include <optional>

class PmdgGroundSource;
class PmdgTabletGateway;

class PmdgGroundConnReconciler
{
public:
    PmdgGroundConnReconciler(PmdgGroundSource& source, PmdgTabletGateway& tablet);

    void SetChocks(bool placed);
    void SetGroundPower(bool on);
    void Reconcile();

private:
    void ReconcileChocks();
    void ReconcileGroundPower();

    PmdgGroundSource& source_;
    PmdgTabletGateway& tablet_;
    std::optional<bool> desiredChocks_;
    std::optional<bool> desiredGroundPower_;
    int chocksAttempts_ = 0;
    int groundPowerAttempts_ = 0;
    int ticksSinceChocksRequest_ = 0;
    int ticksSinceGroundPowerRequest_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNRECONCILER_H
