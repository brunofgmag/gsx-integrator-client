#ifndef GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include "../../domain/model/MemoryBag.h"
#include "../../domain/ports/GsxGateway.h"
#include "GsxRemoteState.h"

class VariableGateway;

class GsxStateService final : public GsxGateway
{
public:
    explicit GsxStateService(VariableGateway* variableGateway, const GsxRemoteState* remoteState = nullptr);

    void Reset();

    [[nodiscard]] MemoryBag TakeMemory() const;
    void RestoreMemory(const MemoryBag& memory, bool gsxRestartedSinceSave);
    [[nodiscard]] bool HaveResumeReadingsArrived() const;

    [[nodiscard]] bool IsAvailable() const;

    void Observe() override;
    [[nodiscard]] GsxStateStatus GetStateStatus(GsxState gsxState) const override;
    [[nodiscard]] bool WasStateCompleted(GsxState gsxState) const override;

    [[nodiscard]] bool IsFuelHoseConnected() const override;
    [[nodiscard]] double GetRefuelCounterGallons() const override;
    [[nodiscard]] bool HasPushbackStarted() const override;
    [[nodiscard]] bool IsPushbackFinished() const override;
    [[nodiscard]] bool IsWaitingForEngines() const override;
    [[nodiscard]] bool IsRepositioning() const override;
    [[nodiscard]] std::optional<bool> HasServiceUnderway() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] int GetBoardedPassengers() override;
    [[nodiscard]] int GetDeboardedPassengers() override;
    [[nodiscard]] double GetBoardingCargoPercent() override;
    [[nodiscard]] bool IsLoadingCargo() const override;
    [[nodiscard]] CargoLoader GetLoaderWaitingForDoor() const override;
    [[nodiscard]] bool IsALoaderAtAHold() const override;
    [[nodiscard]] double GetDeboardingCargoPercent() override;
    [[nodiscard]] bool AreStairsInPlace() const override;
    [[nodiscard]] bool IsJetwayInPlace() const override;
    [[nodiscard]] bool AreStairsAvailable() const override;
    [[nodiscard]] bool IsJetwayAvailable() const override;
    [[nodiscard]] bool IsJetwayOrStairsOperating() const override;
    [[nodiscard]] bool IsServiceVehicleActive() const override;
    [[nodiscard]] bool IsAircraftOnGround() const override;
    [[nodiscard]] double GetGroundSpeedKnots() const override;
    [[nodiscard]] bool IsGoodEngineStartConfirmationEnabled() const override;
    [[nodiscard]] GroundPowerStatus GetGpuStatus() const override;
    [[nodiscard]] bool IsServiceInProgress(GroundService service) const override;
    [[nodiscard]] bool OffersPushback() const override;
    [[nodiscard]] bool IsRemoteApiConnected() const override;
    [[nodiscard]] bool WasGsxDownSinceLastObserve() const override;

    void TakeOverFuelAndPayload() override;
    void OnTurnaroundTurned() override;
    void ReassertTakeovers() const;

    [[nodiscard]] bool IsSimbriefLoaded() const override;
    [[nodiscard]] int GetServedSimbriefGeneration() const override;
    [[nodiscard]] std::string GetSimbriefRefusal() const override;

private:
    bool fuelAndPayloadTakenOver_ = false;
    bool gpuConnectedSeenClear_ = false;
    bool gsxDownSinceLastObserve_ = false;
    struct StateTrack
    {
        enum class Resumption : std::uint8_t
        {
            None,
            OnSameGsx,
            AfterGsxRestart
        };

        GsxStateStatus status = GsxStateStatus::Unavailable;
        bool completed = false;
        bool couatlDiedDuringRun = false;
        bool firstReadingSeen = false;
        bool foundUnderway = false;
        Resumption resumption = Resumption::None;

        [[nodiscard]] bool HadStarted() const;
        [[nodiscard]] bool WasInterruptedByGsxRestart() const;
        [[nodiscard]] bool ReturnedToIdle(GsxStateStatus reading, bool endsWithoutCompleted) const;
        void Save(MemoryBag& memory, const std::string& prefix) const;
        void Load(const MemoryBag& memory, const std::string& prefix);
    };

    struct PassengerCounter
    {
        int last = 0;
        int total = 0;
        bool counting = false;
        bool moved = false;
        bool grown = false;

        int Update(int current, bool active, bool foundUnderway);
        [[nodiscard]] int Reported() const;
        void Save(MemoryBag& memory, const std::string& prefix) const;
        void Load(const MemoryBag& memory, const std::string& prefix);
    };

    struct CargoPercentReading
    {
        double first = 0.0;
        double last = 0.0;
        bool counting = false;
        bool moved = false;

        double Update(double current, bool active, bool foundUnderway);
        [[nodiscard]] double Reported() const;
        void Save(MemoryBag& memory, const std::string& prefix) const;
        void Load(const MemoryBag& memory, const std::string& prefix);
    };

    [[nodiscard]] bool FoundServiceUnderway(GsxState gsxState) const;
    void ObserveState(GsxState gsxState);
    void ObserveGpuConnected();
    [[nodiscard]] int ReadPassengers(PassengerCounter& counter, GsxState gsxState, const char* counterLVar);
    [[nodiscard]] double ReadCargoPercent(CargoPercentReading& reading, GsxState gsxState, const char* percentLVar);
    [[nodiscard]] bool IsAutomationFlagRaised(const char* lVar) const;

    VariableGateway* varManager_;
    const GsxRemoteState* remote_;
    std::map<GsxState, StateTrack> states_;
    PassengerCounter boarding_;
    PassengerCounter deboarding_;
    CargoPercentReading boardingCargo_;
    CargoPercentReading deboardingCargo_;
};
#endif //GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H
