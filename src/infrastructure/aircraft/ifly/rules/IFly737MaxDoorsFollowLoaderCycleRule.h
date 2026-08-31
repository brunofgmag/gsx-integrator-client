#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORSFOLLOWLOADERCYCLERULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORSFOLLOWLOADERCYCLERULE_H

#include <array>

#include "../../../../domain/ports/AircraftRule.h"

class IFly737Max;
class VariableReader;

class IFly737MaxDoorsFollowLoaderCycleRule final : public AircraftRule
{
public:
    IFly737MaxDoorsFollowLoaderCycleRule(VariableReader& variables, const IFly737Max& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    enum class CargoCycle { None, Deboarding, Boarding };

    enum class DoorKind { Cargo, JetwayOrStairs, Stairs, Catering };

    struct Door
    {
        const char* doorName;
        const char* animLVar;
        const char* toggleLVar;
        const char* loaderLVar;
        DoorKind kind = DoorKind::Cargo;
        bool servedSeen = false;
        bool loaderDone = false;
        bool moving = false;
        int attempts = 0;
        int openAttempts = 0;
        int settleTicks = 0;
        int wantsOpenTicks = 0;
        bool pulseHigh = false;
    };

    void FollowAircraftCommands();
    static void ResetTracking(Door& door);
    void ArmCloser(CargoCycle cycle);
    void DisarmCloser();
    void LatchCycleCompletion();
    void TrackDoor(Door& door) const;
    void TrackDoorTravel(Door& door) const;
    void TrackBaggageLoader(Door& door) const;
    bool AdvancePulses(VariableWriter& writer);
    bool AdvancePulse(Door& door, VariableWriter& writer);
    bool Pulse(Door& door, VariableWriter& writer, int& attempts, const char* verb) const;
    [[nodiscard]] CargoCycle CurrentCargoCycle() const;
    [[nodiscard]] bool IsStateActive(const char* stateLVar) const;
    [[nodiscard]] bool IsStateCompleted(const char* stateLVar) const;
    [[nodiscard]] bool HasPendingCargoDoorWork() const;
    [[nodiscard]] bool IsBaggageLoaderPresent(const char* loaderLVar) const;
    [[nodiscard]] bool IsLoaderAtDoorNow(const Door& door) const;
    [[nodiscard]] bool IsDoorReleased(const Door& door) const;
    [[nodiscard]] bool IsDoorCloseable(const Door& door) const;
    [[nodiscard]] bool IsDoorOpenable(const Door& door) const;
    [[nodiscard]] bool IsDoorClosePending(const Door& door) const;
    [[nodiscard]] bool WantsOpen(const Door& door) const;
    [[nodiscard]] std::array<Door*, 6> AllDoors();

    VariableReader* variables_;
    const IFly737Max* aircraft_;
    Door fwdCargoDoor_;
    Door aftCargoDoor_;
    std::array<Door, 4> paxDoors_;
    CargoCycle armedCycle_ = CargoCycle::None;
    bool boardingCompleteSeen_ = false;
    bool deboardingCompleteSeen_ = false;
    bool heldSeen_ = false;
    bool closeRequestSeen_ = false;
    bool closeRequested_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORSFOLLOWLOADERCYCLERULE_H
