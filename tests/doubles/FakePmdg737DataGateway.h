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
    int aircraftModel = 0;
    bool groundPowerAvailable = false;
    bool anyMainBusPowered = false;
    bool groundConnAvailable = false;
    bool beaconOn = false;
    bool parkingBrakeOn = false;
    bool irsAligned = false;
    double totalFuelLbs = 0.0;
    bool inFlight = false;
    std::vector<Pmdg737Door> toggledDoors;

    void Poll() override { ++pollCalls; }
    [[nodiscard]] bool HasData() const override { return hasData; }

    [[nodiscard]] int AircraftModel() const override { return hasData ? aircraftModel : 0; }
    [[nodiscard]] bool GroundPowerAvailable() const override { return hasData && groundPowerAvailable; }
    [[nodiscard]] bool AnyMainBusPowered() const override { return hasData && anyMainBusPowered; }
    [[nodiscard]] bool GroundConnAvailable() const override { return hasData && groundConnAvailable; }
    [[nodiscard]] bool BeaconOn() const override { return hasData && beaconOn; }
    [[nodiscard]] bool ParkingBrakeOn() const override { return hasData && parkingBrakeOn; }
    [[nodiscard]] bool IrsAligned() const override { return hasData && irsAligned; }
    [[nodiscard]] double TotalFuelLbs() const override { return hasData ? totalFuelLbs : 0.0; }

    void ToggleDoor(const Pmdg737Door door) override { toggledDoors.push_back(door); }
    void SetInFlight(const bool value) override { inFlight = value; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG737DATAGATEWAY_H
