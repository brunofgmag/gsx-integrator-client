#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDSFOLLOWTHEIRLOADERRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDSFOLLOWTHEIRLOADERRULE_H

#include <array>
#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class AvroRj;
class GsxDoorSync;
class VariableReader;

class AvroRjHoldsFollowTheirLoaderRule final : public AircraftRule
{
public:
    AvroRjHoldsFollowTheirLoaderRule(VariableReader& variables, const AvroRj& aircraft, GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    struct Hold
    {
        const char* name;
        const char* doorLVar;
        const char* loaderLVar;
        std::optional<bool> pendingTarget;
        int pendingTicks = 0;
        bool commanded = false;
    };

    [[nodiscard]] bool IsLoaderServing(const Hold& hold) const;
    void Follow(Hold& hold, VariableWriter& writer);
    static void Write(const Hold& hold, bool open, const char* reason, VariableWriter& writer);

    VariableReader* variables_;
    const AvroRj* aircraft_;
    GsxDoorSync* doors_;
    std::array<Hold, 2> holds_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJHOLDSFOLLOWTHEIRLOADERRULE_H
