#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777DATAGATEWAY_H

#include <array>
#include <vector>
#include "../../src/infrastructure/pmdg/Pmdg777DataGateway.h"

class FakePmdg777DataGateway final : public Pmdg777DataGateway
{
public:
    bool hasData = false;
    int pollCalls = 0;
    int aircraftModel = 0;
    bool extPowerConnected = false;
    bool extPowerAvailable = false;
    bool beaconOn = false;
    bool parkingBrakeOn = false;
    bool apuRunning = false;
    bool wheelChocksSet = false;
    bool groundConnAvailable = false;
    bool irsAligned = false;
    bool hasFmcFlightPlan = false;
    double totalFuelLbs = 0.0;
    bool inFlight = false;
    int kickCalls = 0;
    std::array<int, 16> doorStates{};
    std::vector<int> toggledDoors;

    FakePmdg777DataGateway()
    {
        doorStates.fill(1);
    }

    void Poll() override { ++pollCalls; }
    [[nodiscard]] bool HasData() const override { return hasData; }

    [[nodiscard]] int AircraftModel() const override { return hasData ? aircraftModel : 0; }
    [[nodiscard]] bool ExtPowerConnected() const override { return hasData && extPowerConnected; }
    [[nodiscard]] bool ExtPowerAvailable() const override { return hasData && extPowerAvailable; }
    [[nodiscard]] bool BeaconOn() const override { return hasData && beaconOn; }
    [[nodiscard]] bool ParkingBrakeOn() const override { return hasData && parkingBrakeOn; }
    [[nodiscard]] bool ApuRunning() const override { return hasData && apuRunning; }
    [[nodiscard]] bool WheelChocksSet() const override { return hasData && wheelChocksSet; }
    [[nodiscard]] bool GroundConnAvailable() const override { return hasData && groundConnAvailable; }
    [[nodiscard]] bool IrsAligned() const override { return hasData && irsAligned; }
    [[nodiscard]] bool HasFmcFlightPlan() const override { return hasData && hasFmcFlightPlan; }
    [[nodiscard]] double TotalFuelLbs() const override { return hasData ? totalFuelLbs : 0.0; }

    [[nodiscard]] int DoorState(const int index) const override
    {
        return hasData && index >= 0 && index < 16 ? doorStates[static_cast<std::size_t>(index)] : -1;
    }

    void ToggleDoor(const int index) override { toggledDoors.push_back(index); }
    void KickDataRefresh() override { ++kickCalls; }
    void SetInFlight(const bool value) override { inFlight = value; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777DATAGATEWAY_H
