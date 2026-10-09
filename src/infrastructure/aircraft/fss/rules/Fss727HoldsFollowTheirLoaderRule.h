#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSFOLLOWTHEIRLOADERRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSFOLLOWTHEIRLOADERRULE_H

#include <array>

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxDoorSync;
class VariableReader;
enum class GsxDoor;

class Fss727HoldsFollowTheirLoaderRule final : public AircraftRule
{
public:
    Fss727HoldsFollowTheirLoaderRule(VariableReader& variables, const Fss727& aircraft, GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    static void MoveTheHold(VariableWriter& writer, GsxDoor door, bool open);
    void CloseTheHoldsOnRequest(VariableWriter& writer);
    [[nodiscard]] bool HasItsLoaderLeft(const char* loaderLVar) const;

    VariableReader* variables_;
    const Fss727* aircraft_;
    GsxDoorSync* doors_;
    std::array<int, 2> servedRequests_{};
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSFOLLOWTHEIRLOADERRULE_H
