#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H

#include <functional>
#include "Pmdg777DataGateway.h"
#include "Pmdg777SdkData.h"
#include "PmdgClientDataChannel.h"

class Pmdg777DataClient final : public Pmdg777DataGateway
{
public:
    Pmdg777DataClient();

    void Poll() override;
    [[nodiscard]] bool HasData() const override;

    [[nodiscard]] bool ExtPowerConnected() const override;
    [[nodiscard]] bool ExtPowerAvailable() const override;
    [[nodiscard]] bool BeaconOn() const override;
    [[nodiscard]] bool ParkingBrakeOn() const override;
    [[nodiscard]] bool ApuRunning() const override;
    [[nodiscard]] bool WheelChocksSet() const override;
    [[nodiscard]] bool HasFmcFlightPlan() const override;
    [[nodiscard]] int DoorState(int index) const override;

    void ToggleDoor(int index) override;
    void KickDataRefresh() override;
    void SetInFlight(bool inFlight) override;

    void SetClockForTest(std::function<long long()> clock) { nowMs_ = std::move(clock); }

private:
    static constexpr long long kKickIntervalMs = 5000;

    PmdgClientDataChannel<PMDG_777X_Data> channel_;
    bool pendingKickRelease_ = false;
    long long lastKickMs_ = -kKickIntervalMs;
    std::function<long long()> nowMs_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H
