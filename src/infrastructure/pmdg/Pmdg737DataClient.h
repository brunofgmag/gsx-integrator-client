#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H

#include <cstddef>
#include "Pmdg737DataGateway.h"
#include "Pmdg737SdkData.h"
#include "PmdgClientDataChannel.h"

class Pmdg737DataClient final : public Pmdg737DataGateway
{
public:
    Pmdg737DataClient();

    void Poll() override;
    [[nodiscard]] bool HasData() const override;

    [[nodiscard]] bool GroundPowerAvailable() const override;
    [[nodiscard]] bool AnyMainBusPowered() const override;
    [[nodiscard]] bool BeaconOn() const override;
    [[nodiscard]] bool ParkingBrakeOn() const override;

    void ToggleDoor(Pmdg737Door door) override;
    void SetInFlight(bool inFlight) override;

    [[nodiscard]] static unsigned DoorEventOffsetFor(Pmdg737Door door);

private:
    static constexpr std::size_t kAcMain1Bus = 11;
    static constexpr std::size_t kAcMain2Bus = 12;

    PmdgClientDataChannel<PMDG_NG3_Data> channel_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATACLIENT_H
