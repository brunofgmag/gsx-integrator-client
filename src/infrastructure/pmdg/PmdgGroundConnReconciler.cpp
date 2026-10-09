#include "PmdgGroundConnReconciler.h"

#include "PmdgGroundSource.h"
#include "PmdgTabletGateway.h"

namespace
{
    constexpr int kGroundConnRetryTicks = 5;
    constexpr int kChocksRetryTicks = 10;
    constexpr int kGroundConnMaxAttempts = 10;

    constexpr auto kChocksRequest = "wheel_chocks";
    constexpr auto kGroundPowerRequest = "ground_power";
    constexpr auto kPassengerEntryRequest = "pax_entree";
    constexpr auto kOwnStairsRequest = "stairs_1l";

    constexpr auto kPendingChocksKey = "pmdgGround.pendingChocks";
    constexpr auto kPendingGroundPowerKey = "pmdgGround.pendingGroundPower";
    constexpr auto kPendingPassengerEntryKey = "pmdgGround.pendingPassengerEntry";

    std::optional<bool> SavedFlag(const MemoryBag& memory, const char* key)
    {
        const bool readAsTrue = memory.Flag(key, true);
        const bool readAsFalse = memory.Flag(key, false);

        return readAsTrue == readAsFalse ? std::optional(readAsTrue) : std::nullopt;
    }
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
        ticksSinceChocksRequest_ = kChocksRetryTicks;
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

void PmdgGroundConnReconciler::AppendMemory(MemoryBag& memory) const
{
    if (desiredChocks_.has_value())
    {
        memory.PutFlag(kPendingChocksKey, *desiredChocks_);
    }

    if (desiredGroundPower_.has_value())
    {
        memory.PutFlag(kPendingGroundPowerKey, *desiredGroundPower_);
    }

    if (passengerEntryRequested_)
    {
        memory.PutFlag(kPendingPassengerEntryKey, true);
    }
}

void PmdgGroundConnReconciler::RestoreMemory(const MemoryBag& memory)
{
    if (const std::optional<bool> chocks = SavedFlag(memory, kPendingChocksKey); chocks.has_value())
    {
        SetChocks(*chocks);
        chocksAwaitTheReading_ = true;
    }

    if (const std::optional<bool> groundPower = SavedFlag(memory, kPendingGroundPowerKey); groundPower.has_value())
    {
        SetGroundPower(*groundPower);
    }

    if (memory.Flag(kPendingPassengerEntryKey, false))
    {
        SetPassengerEntryJetway();
    }
}

void PmdgGroundConnReconciler::ReconcileChocks()
{
    if (!desiredChocks_.has_value())
    {
        return;
    }

    if (chocksAwaitTheReading_)
    {
        if (!source_.ChocksReadingArrived())
        {
            return;
        }

        chocksAwaitTheReading_ = false;
    }

    if (source_.ChocksSet() == *desiredChocks_)
    {
        desiredChocks_.reset();
        chocksAttempts_ = 0;

        return;
    }

    ++ticksSinceChocksRequest_;
    if (ticksSinceChocksRequest_ >= kChocksRetryTicks && chocksAttempts_ < kGroundConnMaxAttempts)
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
        desiredGroundPower_.reset();
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

    const std::optional<bool> jetwayInhibited = tablet_.JetwayInhibited();
    if (!jetwayInhibited.has_value())
    {
        return;
    }

    if (*jetwayInhibited)
    {
        ReleaseOwnStairs();

        return;
    }

    const std::optional<bool> viaJetway = tablet_.PassengerEntryViaJetway();
    if (!viaJetway.has_value())
    {
        return;
    }

    if (*viaJetway)
    {
        passengerEntryRequested_ = false;
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

void PmdgGroundConnReconciler::ReleaseOwnStairs()
{
    const std::optional<bool> deployed = tablet_.OwnStairsDeployed();
    if (!deployed.has_value())
    {
        return;
    }

    if (!*deployed)
    {
        passengerEntryRequested_ = false;
        passengerEntryAttempts_ = 0;

        return;
    }

    ++ticksSincePassengerEntryRequest_;
    if (ticksSincePassengerEntryRequest_ >= kGroundConnRetryTicks
        && passengerEntryAttempts_ < kGroundConnMaxAttempts)
    {
        ticksSincePassengerEntryRequest_ = 0;
        ++passengerEntryAttempts_;
        tablet_.RequestGroundVehicle(kOwnStairsRequest);
    }
}
