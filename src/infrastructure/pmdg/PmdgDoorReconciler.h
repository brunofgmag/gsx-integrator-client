#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRECONCILER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRECONCILER_H

#include <array>
#include <vector>
#include "PmdgDoorSource.h"

enum class DoorBaseline
{
    Unknown,
    Closed
};

class PmdgDoorReconciler
{
public:
    PmdgDoorReconciler(PmdgDoorSource& source, int doorSlots, DoorBaseline baseline);

    void SetDesired(GsxDoor door, bool open);
    void SetSlotDesired(int slot, bool open);
    void Reconcile();
    [[nodiscard]] bool IsStuck(int slot) const;

private:
    void ReconcileSlot(std::size_t slot);
    void CommandDoor(std::size_t slot, bool open, int attempt);

    PmdgDoorSource& source_;
    std::vector<int> desired_;
    std::vector<int> commanded_;
    std::vector<int> ticksSinceCommand_;
    std::vector<int> attempts_;
    std::array<int, static_cast<std::size_t>(GsxDoor::Count)> openedSlot_{};
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRECONCILER_H
