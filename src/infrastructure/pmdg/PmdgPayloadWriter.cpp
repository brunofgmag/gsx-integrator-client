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
    lastRequestedZfwKg_ = 0.0;
    progressiveRampMoving_ = false;
    zfwSettledTicks_ = 0;
    zfwTrims_ = 0;
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
    if (!tablet_.IsAvailable() || !echo.has_value())
    {
        return;
    }

    if (!rampStartZfwKg_.has_value())
    {
        rampStartZfwKg_ = zfwKg;
    }

    const double rampSpanKg = status_->plannedZfwKg - *rampStartZfwKg_;
    const double progress = rampSpanKg > 0.0
                                ? std::clamp((zfwKg - *rampStartZfwKg_) / rampSpanKg, 0.0, 1.0)
                                : 1.0;
    progressiveRampMoving_ = progress < 1.0;

    if (!cargoVariant_)
    {
        const int pax = static_cast<int>(std::lround(progress * status_->plannedPassengers));
        if (pax != lastSentPax_)
        {
            lastSentPax_ = pax;
            tablet_.SendPaxTotal(pax);
        }
    }

    const double nonCargoLbs = echo->zfwLbs - echo->cargoLbs;
    const int cargoLbs =
        (std::max)(static_cast<int>(std::lround(zfwKg * kLbsPerKg - nonCargoLbs)), 0);
    if (cargoLbs != lastProgressiveCargoLbs_)
    {
        lastProgressiveCargoLbs_ = cargoLbs;
        lastSentCargoLbs_ = cargoLbs;
        tablet_.SendCargoTotalLbs(cargoLbs);
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
