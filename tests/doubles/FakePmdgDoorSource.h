#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGDOORSOURCE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGDOORSOURCE_H

#include <map>
#include <vector>
#include "../../src/infrastructure/pmdg/PmdgDoorSource.h"

class FakePmdgDoorSource final : public PmdgDoorSource
{
public:
    std::map<GsxDoor, int> doorSlots;
    std::map<int, DoorObservation> observations;
    std::vector<int> toggled;
    int refreshCalls = 0;

    [[nodiscard]] int DoorSlotFor(const GsxDoor door) const override
    {
        const auto it = doorSlots.find(door);

        return it == doorSlots.end() ? -1 : it->second;
    }

    [[nodiscard]] DoorObservation ObserveDoor(const int slot) const override
    {
        const auto it = observations.find(slot);

        return it == observations.end() ? DoorObservation::Unknown : it->second;
    }

    void ToggleDoor(const int slot) override { toggled.push_back(slot); }
    void RefreshDoors() override { ++refreshCalls; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGDOORSOURCE_H
