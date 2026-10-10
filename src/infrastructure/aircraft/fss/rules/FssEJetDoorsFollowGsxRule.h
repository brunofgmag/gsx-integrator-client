#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H

#include <array>
#include <cstddef>
#include <optional>

#include "../../../../domain/model/MemoryBag.h"
#include "../../../../domain/ports/AircraftRule.h"
#include "../../../gsx/GsxDoorSync.h"

class VariableReader;
class FssEJet;

inline constexpr std::size_t kFssEJetDoorSlotCount = 7;

struct FssEJetDoorSlot
{
    std::optional<GsxDoor> door;
    const char* reqLVar = nullptr;
    const char* ackLVar = nullptr;
    const char* openLVar = nullptr;
    int reaffirmTicks = 0;
};

class FssEJetDoorsFollowGsxRule final : public AircraftRule
{
public:
    FssEJetDoorsFollowGsxRule(VariableReader& variables, GsxDoorSync& doors, const FssEJet& aircraft,
                              bool cargoVariant);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    void AppendMemory(MemoryBag& memory) const;
    void RestoreMemory(const MemoryBag& memory);
    void ReclaimAnOpenMainDeck();

private:
    struct SlotState
    {
        std::optional<bool> desired;
        std::optional<bool> commanded;
        int ticksSinceCommand = 0;
        int attempts = 0;
    };

    void SetDesired(GsxDoor door, bool open);
    void SetMainDeckDesired(bool closeAllPending);
    void ReconcileSlot(std::size_t index, VariableWriter& writer);
    [[nodiscard]] bool IsConfirmed(const FssEJetDoorSlot& slot, bool wantOpen) const;
    [[nodiscard]] bool IsOpenLVarConfirmed(const char* openLVar, bool wantOpen) const;
    static void WriteRequest(const FssEJetDoorSlot& slot, bool open, VariableWriter& writer);
    [[nodiscard]] bool IsMainLoaderWaitingForTheDeck() const;
    [[nodiscard]] bool ClaimsTheOpenMainDeck(bool closeAllPending);
    [[nodiscard]] bool HasTheReadingsTheClaimNeeds() const;
    [[nodiscard]] bool HoldsTheRestoredMainDeckOpen();

    VariableReader* variables_;
    GsxDoorSync* doors_;
    const FssEJet* aircraft_;
    bool cargoVariant_;
    std::array<SlotState, kFssEJetDoorSlotCount> states_{};
    int servedCloseAllRequests_ = 0;
    bool mainDeckRestoredOpen_ = false;
    bool reclaimsTheMainDeck_ = false;
    bool closesTheMainDeck_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETDOORSFOLLOWGSXRULE_H
