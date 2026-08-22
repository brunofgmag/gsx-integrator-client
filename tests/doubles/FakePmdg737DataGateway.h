#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG737DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG737DATAGATEWAY_H

#include <array>
#include <vector>
#include "../../src/infrastructure/pmdg/Pmdg737DataGateway.h"

class FakePmdg737DataGateway final : public Pmdg737DataGateway
{
public:
    bool hasData = false;
    int pollCalls = 0;
    bool groundPowerAvailable = false;
    bool anyMainBusPowered = false;
    bool airstairAnnunciator = false;
    bool beaconOn = false;
    bool parkingBrakeOn = false;
    bool inFlight = false;
    std::vector<Pmdg737Door> toggledDoors;

    void Poll() override { ++pollCalls; }
    [[nodiscard]] bool HasData() const override { return hasData; }

    [[nodiscard]] bool GroundPowerAvailable() const override { return hasData && groundPowerAvailable; }
    [[nodiscard]] bool AnyMainBusPowered() const override { return hasData && anyMainBusPowered; }
    [[nodiscard]] bool BeaconOn() const override { return hasData && beaconOn; }
    [[nodiscard]] bool AirstairAnnunciator() const override { return hasData && airstairAnnunciator; }
    [[nodiscard]] bool ParkingBrakeOn() const override { return hasData && parkingBrakeOn; }

    void ToggleDoor(const Pmdg737Door door) override { toggledDoors.push_back(door); }
    void SetInFlight(const bool value) override { inFlight = value; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG737DATAGATEWAY_H
