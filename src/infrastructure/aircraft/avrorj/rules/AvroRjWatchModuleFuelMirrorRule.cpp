#include "AvroRjWatchModuleFuelMirrorRule.h"

#include <cmath>

#include "../AvroRj.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/SimVars.h"
#include "../../../simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kRuleName = "avro-rj-watch-module-fuel-mirror";

    constexpr auto kModuleFuelMirrorLVar = "146_FuelWeight_KG";
    constexpr double kFuelMovingKgPerTick = 2.0;
    constexpr double kMirrorDivergenceKg = 50.0;
    constexpr int kModuleDeadTicks = 5;
}

AvroRjWatchModuleFuelMirrorRule::AvroRjWatchModuleFuelMirrorRule(VariableReader& variables,
                                                                 AvroRjModuleState& module)
    : variables_(&variables), module_(&module)
{
}

const char* AvroRjWatchModuleFuelMirrorRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjWatchModuleFuelMirrorRule::Evaluate(const RuleContext&)
{
    const double simFuelKg = CurrentFuelKg(*variables_);
    const bool firstSample = lastSimFuelKg_ < 0.0;
    const bool fuelMoving = !firstSample
        && std::abs(simFuelKg - lastSimFuelKg_) > kFuelMovingKgPerTick;
    lastSimFuelKg_ = simFuelKg;

    const double mirrorKg = variables_->GetLVar(kModuleFuelMirrorLVar, 0.0);
    const bool diverged = std::abs(mirrorKg - simFuelKg) > kMirrorDivergenceKg;

    divergentTicks_ = fuelMoving && diverged ? divergentTicks_ + 1 : 0;
    module_->mirroringFuel = divergentTicks_ < kModuleDeadTicks;

    if (!diverged)
    {
        deadLogged_ = false;

        return RuleVerdict::Pass();
    }

    if (!module_->mirroringFuel && !deadLogged_)
    {
        deadLogged_ = true;
        LOG_INFO("The aircraft module stopped mirroring the simulator's fuel; its variables may be frozen");
    }

    return RuleVerdict::Pass();
}

void AvroRjWatchModuleFuelMirrorRule::Act(const RuleContext&, VariableWriter&)
{
}
