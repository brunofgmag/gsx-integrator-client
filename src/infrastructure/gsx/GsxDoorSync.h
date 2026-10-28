#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H

#include <cstdint>
#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>

#include "../../domain/model/MemoryBag.h"

class VariableReader;

enum class GsxDoor : std::uint8_t
{
    FwdPax,
    MidPax,
    AftPax,
    FwdCatering,
    AftCatering,
    FwdCargo,
    AftCargo,
    Count,
};

enum class DoorPosition : std::uint8_t
{
    NotRead,
    Pending,
    Open,
    Closed,
};

class DoorTransit
{
public:
    static constexpr int kBoundTicks = 30;

    void Begin();
    [[nodiscard]] bool IsRunning() const;
    [[nodiscard]] bool Holds(DoorPosition reading, DoorPosition target) const;
    [[nodiscard]] bool Age(DoorPosition reading, DoorPosition target);

private:
    static constexpr int kIdle = -1;

    int ticks_ = kIdle;
};

class GsxDoorSync
{
public:
    using DoorWriter = std::function<void(GsxDoor door, bool open)>;
    using PositionReader = std::function<DoorPosition(GsxDoor door)>;

    explicit GsxDoorSync(VariableReader* variableGateway);

    void WatchExit(GsxDoor door, int exitIndex);
    void ReadPositionsWith(PositionReader reader);
    void Observe();
    void Report() const;
    void Sync(const DoorWriter& write);
    void CloseAll(const DoorWriter& write);
    [[nodiscard]] DoorPosition PositionOf(GsxDoor door) const;
    void HoldClosedForDeparture(bool hold);
    void HoldPassengerDoorsClosed(bool hold);
    [[nodiscard]] double VehicleState(const char* lVar, double absent) const;
    void AppendMemory(MemoryBag& memory) const;
    void RestoreMemory(const MemoryBag& memory, bool gsxRestartedSinceSave);

private:
    struct ExitWatch
    {
        int index = -1;
        double lastPosition = 0.0;
        bool sampled = false;
        bool moving = false;
    };

    [[nodiscard]] bool IsDesiredOpen(GsxDoor door) const;
    [[nodiscard]] bool IsHeldClosed(GsxDoor door) const;
    [[nodiscard]] bool HaveVehiclesArrived(GsxDoor door) const;
    [[nodiscard]] bool IsMoving(GsxDoor door) const;
    [[nodiscard]] bool SettlesWithoutWriting(GsxDoor door, bool wantsOpen);
    [[nodiscard]] DoorPosition OwnTargetPosition(GsxDoor door) const;
    [[nodiscard]] bool IsInTransit(GsxDoor door, DoorPosition position) const;
    void AgeTransit(GsxDoor door);
    void SampleExits();
    void LogHoldOnce(GsxDoor door, const char* reason);
    void RecordTarget(GsxDoor door, bool open);
    void RecordWrite(GsxDoor door, bool open);
    void DistrustEveryVehicle();
    void InheritArrivedVehicles() const;

    VariableReader* variableGateway_;
    PositionReader positionReader_;
    std::array<double, static_cast<std::size_t>(GsxDoor::Count)> lastTargets_{};
    std::array<bool, static_cast<std::size_t>(GsxDoor::Count)> holdLogged_{};
    std::array<bool, static_cast<std::size_t>(GsxDoor::Count)> inheritedTargets_{};
    std::array<ExitWatch, static_cast<std::size_t>(GsxDoor::Count)> exits_{};
    std::array<DoorTransit, static_cast<std::size_t>(GsxDoor::Count)> transits_{};
    bool heldForDeparture_ = false;
    bool passengerDoorsHeld_ = false;
    bool couatlSeenStarted_ = false;
    bool couatlRestarting_ = false;
    mutable std::map<std::string, double> inheritedVehicles_;
    mutable std::set<std::string> awaitingInheritance_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H
