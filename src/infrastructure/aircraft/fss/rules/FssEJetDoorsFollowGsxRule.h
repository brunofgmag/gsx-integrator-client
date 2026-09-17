#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H

#include <array>
#include <optional>

#include "../../../../domain/ports/AircraftRule.h"
#include "../../../gsx/GsxDoorSync.h"

class VariableReader;
class FssEJet;

struct FssEJetDoorSlot
{
    GsxDoor door;
    const char* reqLVar;
    const char* ackLVar;
    const char* openLVar;
    int reaffirmTicks;
};

class FssEJetDoorsFollowGsxRule final : public AircraftRule
{
public:
    FssEJetDoorsFollowGsxRule(VariableReader& variables, GsxDoorSync& doors, const FssEJet& aircraft,
                              bool cargoVariant);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    void RequestCloseAll();

private:
    struct SlotState
    {
        std::optional<bool> desired;
        std::optional<bool> commanded;
        int ticksSinceCommand = 0;
        int attempts = 0;
    };

    void SetDesired(GsxDoor door, bool open);
    void ReconcileSlot(std::size_t index, VariableWriter& writer);
    [[nodiscard]] bool IsConfirmed(const FssEJetDoorSlot& slot, bool wantOpen) const;
    [[nodiscard]] bool IsOpenLVarConfirmed(const char* openLVar, bool wantOpen) const;
    static void WriteRequest(const FssEJetDoorSlot& slot, bool open, VariableWriter& writer);

    void ReconcileMainDeck(bool forceClosed, VariableWriter& writer);
    static void WriteMainDeckRequest(bool open, VariableWriter& writer);
    [[nodiscard]] bool IsMainLoaderWaitingForTheDeck() const;
    [[nodiscard]] bool IsAircraftEnergized() const;

    VariableReader* variables_;
    GsxDoorSync* doors_;
    const FssEJet* aircraft_;
    bool cargoVariant_;
    std::array<SlotState, 6> states_{};
    int closeAllRequests_ = 0;
    int servedCloseAllRequests_ = 0;

    std::optional<bool> mainDeckCommanded_;
    int mainDeckTicksSinceCommand_ = 0;
    int mainDeckAttempts_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H
