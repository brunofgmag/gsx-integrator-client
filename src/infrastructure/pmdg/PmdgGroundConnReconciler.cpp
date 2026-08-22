#include "PmdgGroundConnReconciler.h"

#include "PmdgGroundSource.h"
#include "PmdgTabletGateway.h"

namespace
{
    constexpr int kGroundConnRetryTicks = 5;
    constexpr int kGroundConnMaxAttempts = 10;

    constexpr auto kChocksRequest = "wheel_chocks";
    constexpr auto kGroundPowerRequest = "ground_power";
    constexpr auto kPassengerEntryRequest = "pax_entree";
}

PmdgGroundConnReconciler::PmdgGroundConnReconciler(PmdgGroundSource& source, PmdgTabletGateway& tablet)
    : source_(source),
      tablet_(tablet)
{
}

void PmdgGroundConnReconciler::SetChocks(const bool placed)
{
    if (desiredChocks_ != placed)
    {
        desiredChocks_ = placed;
        chocksAttempts_ = 0;
        ticksSinceChocksRequest_ = kGroundConnRetryTicks;
    }
}

void PmdgGroundConnReconciler::SetGroundPower(const bool on)
{
    if (desiredGroundPower_ != on)
    {
        desiredGroundPower_ = on;
        groundPowerAttempts_ = 0;
        ticksSinceGroundPowerRequest_ = kGroundConnRetryTicks;
    }
}

void PmdgGroundConnReconciler::SetPassengerEntryJetway()
{
    if (passengerEntryRequested_)
    {
        return;
    }

    passengerEntryRequested_ = true;
    passengerEntryAttempts_ = 0;
    ticksSincePassengerEntryRequest_ = kGroundConnRetryTicks;
}

void PmdgGroundConnReconciler::Reconcile()
{
    ReconcileChocks();
    ReconcileGroundPower();
    ReconcilePassengerEntry();
}

void PmdgGroundConnReconciler::ReconcileChocks()
{
    if (!desiredChocks_.has_value() || source_.ChocksSet() == *desiredChocks_)
    {
        chocksAttempts_ = 0;

        return;
    }

    ++ticksSinceChocksRequest_;
    if (ticksSinceChocksRequest_ >= kGroundConnRetryTicks && chocksAttempts_ < kGroundConnMaxAttempts)
    {
        ticksSinceChocksRequest_ = 0;
        ++chocksAttempts_;
        tablet_.RequestGroundConn(kChocksRequest);
    }
}

void PmdgGroundConnReconciler::ReconcileGroundPower()
{
    if (!desiredGroundPower_.has_value())
    {
        return;
    }

    if (source_.GroundPowerPresent() == *desiredGroundPower_)
    {
        groundPowerAttempts_ = 0;

        return;
    }

    if (tablet_.GroundConnMoving(kGroundPowerRequest))
    {
        ticksSinceGroundPowerRequest_ = 0;

        return;
    }

    ++ticksSinceGroundPowerRequest_;
    if (ticksSinceGroundPowerRequest_ >= kGroundConnRetryTicks
        && groundPowerAttempts_ < kGroundConnMaxAttempts)
    {
        ticksSinceGroundPowerRequest_ = 0;
        ++groundPowerAttempts_;
        tablet_.RequestGroundConn(kGroundPowerRequest);
    }
}

void PmdgGroundConnReconciler::ReconcilePassengerEntry()
{
    if (!passengerEntryRequested_)
    {
        return;
    }

    const std::optional<bool> viaJetway = tablet_.PassengerEntryViaJetway();
    if (!viaJetway.has_value())
    {
        return;
    }

    if (*viaJetway)
    {
        passengerEntryAttempts_ = 0;

        return;
    }

    ++ticksSincePassengerEntryRequest_;
    if (ticksSincePassengerEntryRequest_ >= kGroundConnRetryTicks
        && passengerEntryAttempts_ < kGroundConnMaxAttempts)
    {
        ticksSincePassengerEntryRequest_ = 0;
        ++passengerEntryAttempts_;
        tablet_.RequestGroundConn(kPassengerEntryRequest);
    }
}
