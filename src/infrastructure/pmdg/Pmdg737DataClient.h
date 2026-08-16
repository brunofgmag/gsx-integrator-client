#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H

#include <cstddef>
#include "Pmdg737DataGateway.h"
#include "Pmdg737SdkData.h"
#include "../simconnect/SimConnectSession.h"

class Pmdg737DataClient final : public Pmdg737DataGateway
{
public:
    Pmdg737DataClient();
    ~Pmdg737DataClient() override;

    void Poll() override;
    [[nodiscard]] bool HasData() const override;

    [[nodiscard]] int AircraftModel() const override;
    [[nodiscard]] bool GroundPowerAvailable() const override;
    [[nodiscard]] bool AnyMainBusPowered() const override;
    [[nodiscard]] bool GroundConnAvailable() const override;
    [[nodiscard]] bool BeaconOn() const override;
    [[nodiscard]] bool ParkingBrakeOn() const override;
    [[nodiscard]] bool IrsAligned() const override;
    [[nodiscard]] double TotalFuelLbs() const override;
    [[nodiscard]] bool DoorOpen(Pmdg737Door door) const override;

    void ToggleDoor(Pmdg737Door door) override;
    void SetInFlight(bool inFlight) override;

    [[nodiscard]] static unsigned DoorEventOffsetFor(Pmdg737Door door);
    [[nodiscard]] static bool HasAnnunciator(Pmdg737Door door);
    [[nodiscard]] SimConnectSession& SessionForTest() { return session_; }

private:
    void EnsureConnected();
    void OnClientData(const void* data, DWORD size);

    static constexpr std::size_t kAcMain1Bus = 11;
    static constexpr std::size_t kAcMain2Bus = 12;

    SimConnectSession session_;
    PMDG_NG3_Data data_{};
    bool hasData_ = false;
    bool connected_ = false;
    bool inFlight_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H
