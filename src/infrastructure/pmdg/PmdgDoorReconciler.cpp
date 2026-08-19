#include "PmdgDoorReconciler.h"

namespace
{
    constexpr int kDoorRetryTicks = 5;
    constexpr int kDoorMaxAttempts = 2;
}

PmdgDoorReconciler::PmdgDoorReconciler(PmdgDoorSource& source, const int doorSlots,
                                       const DoorBaseline baseline)
    : source_(source),
      desired_(static_cast<std::size_t>(doorSlots), -1),
      commanded_(static_cast<std::size_t>(doorSlots), baseline == DoorBaseline::Closed ? 0 : -1),
      ticksSinceCommand_(static_cast<std::size_t>(doorSlots), 0),
      attempts_(static_cast<std::size_t>(doorSlots), 0)
{
    openedSlot_.fill(-1);
}

void PmdgDoorReconciler::SetDesired(const GsxDoor door, const bool open)
{
    int& openedSlot = openedSlot_[static_cast<std::size_t>(door)];
    const int slot = !open && openedSlot >= 0 ? openedSlot : source_.DoorSlotFor(door);
    if (slot < 0)
    {
        return;
    }

    desired_[static_cast<std::size_t>(slot)] = open ? 1 : 0;
    openedSlot = open ? slot : -1;
}

void PmdgDoorReconciler::SetSlotDesired(const int slot, const bool open)
{
    if (slot < 0 || static_cast<std::size_t>(slot) >= desired_.size())
    {
        return;
    }

    desired_[static_cast<std::size_t>(slot)] = open ? 1 : 0;
}

void PmdgDoorReconciler::Reconcile()
{
    for (std::size_t slot = 0; slot < desired_.size(); ++slot)
    {
        ReconcileSlot(slot);
    }
}

void PmdgDoorReconciler::ReconcileSlot(const std::size_t slot)
{
    if (desired_[slot] < 0)
    {
        return;
    }

    const DoorObservation observation = source_.ObserveDoor(static_cast<int>(slot));
    if (observation == DoorObservation::Unavailable || observation == DoorObservation::Moving)
    {
        return;
    }

    const bool wantOpen = desired_[slot] == 1;
    const bool known = observation != DoorObservation::Unknown;
    const bool isOpen = known ? observation == DoorObservation::Open : commanded_[slot] == 1;

    if (commanded_[slot] != desired_[slot])
    {
        commanded_[slot] = desired_[slot];
        ticksSinceCommand_[slot] = 0;
        attempts_[slot] = 0;

        if (isOpen != wantOpen)
        {
            source_.ToggleDoor(static_cast<int>(slot));
        }

        return;
    }

    if (!known || isOpen == wantOpen)
    {
        attempts_[slot] = 0;

        return;
    }

    ++ticksSinceCommand_[slot];
    if (ticksSinceCommand_[slot] >= kDoorRetryTicks && attempts_[slot] < kDoorMaxAttempts)
    {
        ticksSinceCommand_[slot] = 0;
        ++attempts_[slot];
        source_.ToggleDoor(static_cast<int>(slot));
    }
}

bool PmdgDoorReconciler::IsStuck(const int slot) const
{
    if (slot < 0 || static_cast<std::size_t>(slot) >= desired_.size())
    {
        return false;
    }

    const auto index = static_cast<std::size_t>(slot);

    return desired_[index] == 1 && attempts_[index] >= kDoorMaxAttempts;
}
