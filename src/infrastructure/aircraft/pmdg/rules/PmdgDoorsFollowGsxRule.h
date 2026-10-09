#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSFOLLOWGSXRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class GsxDoorSync;
class PmdgDataGateway;
class PmdgDoorReconciler;

class PmdgDoorsFollowGsxRule final : public AircraftRule
{
public:
    PmdgDoorsFollowGsxRule(const PmdgDataGateway& data, GsxDoorSync& doors, PmdgDoorReconciler& reconciler,
                           bool cargoVariant, int mainDeckDoorSlot);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    void SyncMainDeckDoor();

    const PmdgDataGateway* data_;
    GsxDoorSync* doors_;
    PmdgDoorReconciler* reconciler_;
    bool cargoVariant_;
    int mainDeckDoorSlot_;
    bool mainDeckDoorTaken_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSFOLLOWGSXRULE_H
