#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class FenixA32xDisarmRefuelWhenDoneRule final : public AircraftRule
{
public:
    explicit FenixA32xDisarmRefuelWhenDoneRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    void ResumeLoading(bool refuelFinished, bool gsxRestartedSinceSave);

private:
    [[nodiscard]] bool RefuelHasEnded() const;
    [[nodiscard]] bool GsxLeftTheFinishedServiceBehind() const;
    [[nodiscard]] bool RefuelServiceReadsCallable() const;
    void CountTheCallableReading();
    [[nodiscard]] std::optional<bool> ReadTheHoseArmed() const;

    VariableReader* variables_;
    bool armed_ = false;
    bool loadingSeen_ = false;
    bool resumed_ = false;
    bool refuelFinishedBeforeTheResume_ = false;
    bool gsxRestartedSinceSave_ = false;
    int callableTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H
