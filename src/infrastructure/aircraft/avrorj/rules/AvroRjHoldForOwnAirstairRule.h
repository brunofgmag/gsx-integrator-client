#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDFOROWNAIRSTAIRRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDFOROWNAIRSTAIRRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class AvroRj;
class GsxDoorSync;
class VariableReader;
struct AvroRjAirstairState;

class AvroRjHoldForOwnAirstairRule final : public AircraftRule
{
public:
    AvroRjHoldForOwnAirstairRule(VariableReader& variables, const AvroRj& aircraft,
                                 GsxDoorSync& doors, AvroRjAirstairState& airstair);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    enum class Phase
    {
        Stowed,
        Arming,
        Extended,
        Unarming
    };

    void ObserveTravel();
    [[nodiscard]] bool IsWanted() const;
    [[nodiscard]] bool IsOutOfItsWell() const;
    [[nodiscard]] bool IsMoving() const;
    [[nodiscard]] bool HasPressure() const;
    bool PressureReady();
    void Drive(VariableWriter& writer);

    VariableReader* variables_;
    const AvroRj* aircraft_;
    GsxDoorSync* doors_;
    AvroRjAirstairState* airstair_;
    Phase phase_ = Phase::Stowed;
    int positionStillTicks_ = 0;
    bool pressureWaitLogged_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDFOROWNAIRSTAIRRULE_H
