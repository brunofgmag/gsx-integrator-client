#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJPAXDOORSSERVETHEAIRSTAIRRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJPAXDOORSSERVETHEAIRSTAIRRULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class AvroRj;
class GsxDoorSync;
class VariableReader;
struct AvroRjAirstairState;

class AvroRjPaxDoorsServeTheAirstairRule final : public AircraftRule
{
public:
    AvroRjPaxDoorsServeTheAirstairRule(VariableReader& variables, const AvroRj& aircraft,
                                       GsxDoorSync& doors, const AvroRjAirstairState& airstair);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    [[nodiscard]] std::optional<double> FrontDoorTarget() const;
    void RestoreFrontDoorTarget(const std::optional<double>& target);

private:
    [[nodiscard]] bool IsFrontDoorWanted() const;
    [[nodiscard]] bool AwaitsTheVehiclesOfARestoredTarget();
    [[nodiscard]] const char* WhatServesTheFrontDoor() const;
    void DriveFrontDoor(VariableWriter& writer);
    void CommandFrontDoor(VariableWriter& writer, double target);
    void KeepAftDoorClosed(VariableWriter& writer);

    VariableReader* variables_;
    const AvroRj* aircraft_;
    GsxDoorSync* doors_;
    const AvroRjAirstairState* airstair_;
    std::optional<double> lastFrontDoorTarget_;
    bool restoredOpenTargetAwaitsVehicles_ = false;
    bool aftDoorCloseWritten_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJPAXDOORSSERVETHEAIRSTAIRRULE_H
