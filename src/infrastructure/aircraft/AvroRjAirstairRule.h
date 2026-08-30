#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJAIRSTAIRRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJAIRSTAIRRULE_H

#include "../../domain/ports/AircraftRule.h"

class AvroRj;

class AvroRjAirstairRule final : public AircraftRule
{
public:
    explicit AvroRjAirstairRule(AvroRj& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter&) override;

private:
    AvroRj* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJAIRSTAIRRULE_H
