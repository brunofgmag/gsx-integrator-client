#include "PmdgPayloadWriter.h"

#include <algorithm>
#include <cmath>
#include "PmdgTabletGateway.h"
#include "../simvars/SimVars.h"
#include "../../domain/model/AutomationStatus.h"

using namespace simvars;

namespace
{
    constexpr double kLbsPerKg = 2.20462262185;
    constexpr int kEfbHoldRoundingLbs = 3;

    constexpr int kZfwSettleTicks = 5;
    constexpr int kZfwTrimMaxAttempts = 5;
    constexpr double kZfwTrimToleranceKg = 50.0;
}

PmdgPayloadWriter::PmdgPayloadWriter(PmdgTabletGateway& tablet, VariableReader& variables,
                                     const AutomationStatus* status, const bool cargoVariant)
    : tablet_(tablet),
      variables_(variables),
      status_(status),
      cargoVariant_(cargoVariant)
{
}

void PmdgPayloadWriter::Reset()
{
    lastSentFuelLbs_ = -1;
    lastSentPax_ = -1;
    lastSentCargoLbs_ = -1;
    lastProgressiveCargoLbs_ = -1;
    rampStartZfwKg_.reset();
    resumed_ = false;
    resumedPlannedZfwKg_.reset();
    resumedPlannedPassengers_.reset();
    rememberedEcho_.reset();
    perPassengerLbs_.reset();
    lastRequestedZfwKg_ = 0.0;
    progressiveRampMoving_ = false;
    zfwSettledTicks_ = 0;
    zfwTrims_ = 0;
}

void PmdgPayloadWriter::Resume(const double emptyZfwKg, const double plannedZfwKg, const int plannedPassengers)
{
    resumed_ = true;

    if (emptyZfwKg > 0.0)
    {
        rampStartZfwKg_ = emptyZfwKg;
    }

    if (plannedZfwKg > 0.0)
    {
        resumedPlannedZfwKg_ = plannedZfwKg;
        resumedPlannedPassengers_ = plannedPassengers;
    }
}

bool PmdgPayloadWriter::LivePlanExists() const
{
    return status_->plannedZfwKg > 0.0;
}

double PmdgPayloadWriter::PlannedZfwKg() const
{
    return LivePlanExists() ? status_->plannedZfwKg : resumedPlannedZfwKg_.value_or(status_->plannedZfwKg);
}

int PmdgPayloadWriter::PlannedPassengers() const
{
    return LivePlanExists() ? status_->plannedPassengers
                            : resumedPlannedPassengers_.value_or(status_->plannedPassengers);
}

void PmdgPayloadWriter::LearnPassengerWeight(const PmdgWeightEcho& echo)
{
    const double nonCargoLbs = echo.zfwLbs - echo.cargoLbs;
    if (!rememberedEcho_.has_value())
    {
        rememberedEcho_ = PassengerSample{echo.passengers, nonCargoLbs};

        return;
    }

    if (echo.passengers == rememberedEcho_->passengers)
    {
        return;
    }

    const double lbsPerPassenger =
        (nonCargoLbs - rememberedEcho_->nonCargoLbs) / (echo.passengers - rememberedEcho_->passengers);
    if (lbsPerPassenger > 0.0)
    {
        perPassengerLbs_ = lbsPerPassenger;
    }

    rememberedEcho_ = PassengerSample{echo.passengers, nonCargoLbs};
}

std::optional<double> PmdgPayloadWriter::NonCargoLbsOnceSentPassengersBoard(const PmdgWeightEcho& echo) const
{
    const double nonCargoLbs = echo.zfwLbs - echo.cargoLbs;
    if (cargoVariant_)
    {
        return nonCargoLbs;
    }

    if (!perPassengerLbs_.has_value())
    {
        return echo.passengers == lastSentPax_ ? std::optional(nonCargoLbs) : std::nullopt;
    }

    return nonCargoLbs + (lastSentPax_ - echo.passengers) * *perPassengerLbs_;
}

void PmdgPayloadWriter::SendCargoFor(const double zfwKg, const double nonCargoLbs)
{
    const int cargoLbs =
        (std::max)(static_cast<int>(std::lround((zfwKg * kLbsPerKg) - nonCargoLbs)), 0);
    if (lastProgressiveCargoLbs_ < 0 || std::abs(cargoLbs - lastProgressiveCargoLbs_) >= kEfbHoldRoundingLbs)
    {
        lastProgressiveCargoLbs_ = cargoLbs;
        lastSentCargoLbs_ = cargoLbs;
        tablet_.SendCargoTotalLbs(cargoLbs);
    }
}

void PmdgPayloadWriter::SetFuelKg(const double fuelKg)
{
    if (!tablet_.IsAvailable())
    {
        return;
    }

    const int lbs = static_cast<int>(std::lround(fuelKg * kLbsPerKg));
    if (lbs == lastSentFuelLbs_)
    {
        return;
    }

    lastSentFuelLbs_ = lbs;
    tablet_.SendFuelTotalLbs(lbs);
}

void PmdgPayloadWriter::SetZfwKg(const double zfwKg)
{
    const std::optional<PmdgWeightEcho> echo = tablet_.LastWeightEcho();
    const double plannedZfwKg = PlannedZfwKg();
    if (!tablet_.IsAvailable() || !echo.has_value() || (resumed_ && plannedZfwKg <= 0.0))
    {
        return;
    }

    if (!rampStartZfwKg_.has_value())
    {
        rampStartZfwKg_ = zfwKg;
    }

    const double rampSpanKg = plannedZfwKg - *rampStartZfwKg_;
    const double progress = rampSpanKg > 0.0
                                ? std::clamp((zfwKg - *rampStartZfwKg_) / rampSpanKg, 0.0, 1.0)
                                : 1.0;
    progressiveRampMoving_ = progress < 1.0;

    if (!cargoVariant_)
    {
        LearnPassengerWeight(*echo);

        const int pax = static_cast<int>(std::lround(progress * PlannedPassengers()));
        if (pax != lastSentPax_)
        {
            lastSentPax_ = pax;
            tablet_.SendPaxTotal(pax);
        }
    }

    if (const std::optional<double> nonCargoLbs = NonCargoLbsOnceSentPassengersBoard(*echo); nonCargoLbs.has_value())
    {
        SendCargoFor(zfwKg, *nonCargoLbs);
    }

    if (lastRequestedZfwKg_ != zfwKg)
    {
        lastRequestedZfwKg_ = zfwKg;
        zfwSettledTicks_ = 0;
        zfwTrims_ = 0;
    }
}

void PmdgPayloadWriter::Trim()
{
    if (lastRequestedZfwKg_ <= 0.0 || lastSentCargoLbs_ < 0 || !tablet_.IsAvailable()
        || progressiveRampMoving_ || zfwTrims_ >= kZfwTrimMaxAttempts)
    {
        return;
    }

    if (++zfwSettledTicks_ < kZfwSettleTicks)
    {
        return;
    }

    const double errorKg = CurrentZfwKg(variables_) - lastRequestedZfwKg_;
    if (std::abs(errorKg) <= kZfwTrimToleranceKg)
    {
        return;
    }

    const int trimmedLbs =
        (std::max)(lastSentCargoLbs_ - static_cast<int>(std::lround(errorKg * kLbsPerKg)), 0);
    if (trimmedLbs == lastSentCargoLbs_)
    {
        zfwTrims_ = kZfwTrimMaxAttempts;
        return;
    }

    zfwSettledTicks_ = 0;
    ++zfwTrims_;
    lastSentCargoLbs_ = trimmedLbs;
    tablet_.SendCargoTotalLbs(trimmedLbs);
}
