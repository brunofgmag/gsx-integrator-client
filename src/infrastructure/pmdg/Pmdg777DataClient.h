#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H

#include <functional>
#include "Pmdg777DataGateway.h"
#include "Pmdg777SdkData.h"
#include "../simconnect/SimConnectSession.h"

class Pmdg777DataClient final : public Pmdg777DataGateway
{
public:
    Pmdg777DataClient();
    ~Pmdg777DataClient() override;

    void Poll() override;
    [[nodiscard]] bool HasData() const override;

    [[nodiscard]] int AircraftModel() const override;
    [[nodiscard]] bool ExtPowerConnected() const override;
    [[nodiscard]] bool ExtPowerAvailable() const override;
    [[nodiscard]] bool BeaconOn() const override;
    [[nodiscard]] bool ParkingBrakeOn() const override;
    [[nodiscard]] bool ApuRunning() const override;
    [[nodiscard]] bool WheelChocksSet() const override;
    [[nodiscard]] bool GroundConnAvailable() const override;
    [[nodiscard]] bool IrsAligned() const override;
    [[nodiscard]] bool HasFmcFlightPlan() const override;
    [[nodiscard]] double TotalFuelLbs() const override;
    [[nodiscard]] int DoorState(int index) const override;

    void ToggleDoor(int index) override;
    void KickDataRefresh() override;
    void SetInFlight(bool inFlight) override;

    void SetClockForTest(std::function<long long()> clock) { nowMs_ = std::move(clock); }
    [[nodiscard]] SimConnectSession& SessionForTest() { return session_; }

private:
    void EnsureConnected();
    void OnClientData(const void* data, DWORD size);

    static constexpr long long kKickIntervalMs = 5000;

    SimConnectSession session_;
    PMDG_777X_Data data_{};
    bool hasData_ = false;
    bool connected_ = false;
    bool inFlight_ = false;
    bool pendingKickRelease_ = false;
    long long lastKickMs_ = -kKickIntervalMs;
    std::function<long long()> nowMs_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATACLIENT_H
