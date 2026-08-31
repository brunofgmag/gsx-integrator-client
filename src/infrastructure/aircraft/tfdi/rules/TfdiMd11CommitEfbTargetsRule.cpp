#include "TfdiMd11CommitEfbTargetsRule.h"

#include <algorithm>
#include <optional>

#include "../TfdiMd11.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/SimVars.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/support/Weight.h"

using namespace simvars;

namespace
{
    constexpr auto kRuleName = "tfdi-md11-commit-efb-targets";

    constexpr auto kEfbGw = "MD11_EFB_PAYLOAD_GW";
    constexpr auto kEfbZfw = "MD11_EFB_PAYLOAD_ZFW";
    constexpr auto kEfbPayload = "MD11_EFB_PAYLOAD_PAYLOAD";
    constexpr auto kEfbFuel = "MD11_EFB_PAYLOAD_FUEL";
    constexpr auto kEfbLoad = "MD11_EFB_PAYLOAD_LOAD";
    constexpr auto kEfbReadReady = "MD11_EFB_READ_READY";

    constexpr int kEfbReadyMask = 0x1;
    constexpr double kMtowKg = 283730.0;
}

TfdiMd11CommitEfbTargetsRule::TfdiMd11CommitEfbTargetsRule(VariableReader& variables, const TfdiMd11& aircraft)
    : variables_(&variables), aircraft_(&aircraft)
{
}

const char* TfdiMd11CommitEfbTargetsRule::Name() const
{
    return kRuleName;
}

RuleCadence TfdiMd11CommitEfbTargetsRule::Cadence() const
{
    return RuleCadence::Slow;
}

RuleVerdict TfdiMd11CommitEfbTargetsRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11CommitEfbTargetsRule::Act(const RuleContext&, VariableWriter& writer)
{
    const std::optional<double> stagedFuelKg = aircraft_->StagedFuelKg();
    const std::optional<double> stagedZfwKg = aircraft_->StagedZfwKg();

    if (!stagedFuelKg.has_value() && !stagedZfwKg.has_value())
    {
        return;
    }

    if (!variables_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return;
    }

    if (stagedFuelKg.has_value())
    {
        fuelTarget_ = {*stagedFuelKg, true};
    }
    else
    {
        SeedFuelIfNeeded();
    }

    if (stagedZfwKg.has_value())
    {
        zfwTarget_ = {*stagedZfwKg, true};
    }
    else
    {
        SeedZfwIfNeeded();
    }

    if (fuelTarget_.value == committedFuelKg_ && zfwTarget_.value == committedZfwKg_)
    {
        return;
    }

    CommitTargets(writer);

    committedFuelKg_ = fuelTarget_.value;
    committedZfwKg_ = zfwTarget_.value;
}

void TfdiMd11CommitEfbTargetsRule::SeedFuelIfNeeded()
{
    if (fuelTarget_.seeded || !variables_->HasReceivedAVar(kSimFuelTotalKg, kKgUnit))
    {
        return;
    }

    fuelTarget_ = {aircraft_->GetCurrentFuelKg(), true};
}

void TfdiMd11CommitEfbTargetsRule::SeedZfwIfNeeded()
{
    if (zfwTarget_.seeded || !variables_->HasReceivedAVar(kSimTotalWeight, kKgUnit))
    {
        return;
    }

    zfwTarget_ = {std::max(aircraft_->GetCurrentZfwKg(), aircraft_->GetEmptyZfwKg()), true};
}

void TfdiMd11CommitEfbTargetsRule::CommitTargets(VariableWriter& writer) const
{
    const double emptyZfwKg = aircraft_->GetEmptyZfwKg();
    const double zfw = std::max(zfwTarget_.value, emptyZfwKg);
    const double fuel = std::max(fuelTarget_.value, 0.0);
    const double payload = std::max(zfw - emptyZfwKg, 0.0);
    const double grossWeight = zfw + fuel;
    const double payloadCapacityKg = kMtowKg - emptyZfwKg;
    const double loadPercentage = std::clamp(
        payload / payloadCapacityKg * 100.0,
        0.0,
        100.0);

    writer.SetLVar(kEfbGw, weight::KgToLb(grossWeight));
    writer.SetLVar(kEfbZfw, weight::KgToLb(zfw));
    writer.SetLVar(kEfbPayload, weight::KgToLb(payload));
    writer.SetLVar(kEfbFuel, weight::KgToLb(fuel));
    writer.SetLVar(kEfbLoad, loadPercentage);

    const int flags = static_cast<int>(variables_->GetLVar(kEfbReadReady, 0.0)) | kEfbReadyMask;
    writer.SetLVar(kEfbReadReady, flags);

    LOG_INFO(
        "Committed EFB targets: fuel=%.0fkg zfw=%.0fkg payload=%.0fkg",
        fuel,
        zfw,
        payload);
}
