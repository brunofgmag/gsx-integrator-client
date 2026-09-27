#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETGPUFOLLOWSREQUESTRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETGPUFOLLOWSREQUESTRULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class FssEJet;

class FssEJetGpuFollowsRequestRule final : public AircraftRule
{
public:
    explicit FssEJetGpuFollowsRequestRule(const FssEJet& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    void ServeNewRequest();

    const FssEJet* aircraft_;
    std::optional<bool> desired_;
    int attempts_ = 0;
    int ticksSincePulse_;
    int servedGroundPowerRequests_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETGPUFOLLOWSREQUESTRULE_H
