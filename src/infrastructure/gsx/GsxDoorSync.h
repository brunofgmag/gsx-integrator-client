#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H

#include <array>
#include <functional>
#include <map>
#include <string>

class VariableReader;

enum class GsxDoor
{
    FwdPax,
    MidPax,
    AftPax,
    FwdCatering,
    AftCatering,
    FwdCargo,
    AftCargo,
    Count
};

class GsxDoorSync
{
public:
    using DoorWriter = std::function<void(GsxDoor door, bool open)>;

    explicit GsxDoorSync(VariableReader* variableGateway);

    void WatchExit(GsxDoor door, int exitIndex);
    void Observe();
    void Report() const;
    void Sync(const DoorWriter& write);
    void CloseAll(const DoorWriter& write);
    void HoldClosedForDeparture(bool hold);
    [[nodiscard]] double VehicleState(const char* lVar, double absent) const;

private:
    struct ExitWatch
    {
        int index = -1;
        double lastPosition = 0.0;
        bool sampled = false;
        bool moving = false;
    };

    [[nodiscard]] bool IsDesiredOpen(GsxDoor door) const;
    [[nodiscard]] bool IsMoving(GsxDoor door) const;
    void SampleExits();

    VariableReader* variableGateway_;
    std::array<double, static_cast<std::size_t>(GsxDoor::Count)> lastTargets_{};
    std::array<ExitWatch, static_cast<std::size_t>(GsxDoor::Count)> exits_{};
    bool heldForDeparture_ = false;
    bool couatlSeenStarted_ = false;
    bool couatlRestarting_ = false;
    mutable std::map<std::string, double> inheritedVehicles_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXDOORSYNC_H
