#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSAOC_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSAOC_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include "../../fslabs/FslabsMcduGateway.h"

struct AocScript;

enum class AocRequest : std::uint8_t { SendOfp, Board, Pushback };

class FslabsAoc
{
public:
    explicit FslabsAoc(FslabsMcduGateway& mcdu);

    void Advance(bool initIsAwaited, bool powerIsAsked);
    [[nodiscard]] bool IsAlive() const;
    [[nodiscard]] bool IsInitDone() const;
    [[nodiscard]] bool Request(AocRequest request);
    void ResetForNewTurnaround();

private:
    struct Completion
    {
        long long tick = 0;
        bool reported = false;
    };

    struct Run
    {
        const AocScript* script = nullptr;
        std::size_t step = 0;
        std::uint64_t serialAtPress = 0;
        std::string titleBeforePress;
    };

    void Start(const AocScript& script);
    void StartNext(bool initIsAwaited, bool powerIsAsked);
    void ContinueRun();
    void PressNext();
    void RememberTheCompletion();
    void RecordArrival(bool pageChanged, const FslabsMcduScreen& screen);
    void Abort(const FslabsMcduScreen& screen);
    void ReportAbort(const FslabsMcduScreen& screen);
    void LeaveTheMenu();
    void EndRun();
    void ReportLifeChange();
    [[nodiscard]] bool McduShowsAScreen() const;
    [[nodiscard]] bool InitIsPending(bool initIsAwaited) const;
    [[nodiscard]] bool ProbeIsDue(bool initIsAwaited, bool powerIsAsked) const;
    [[nodiscard]] bool IsRunningOrQueued(const AocScript& script) const;
    [[nodiscard]] bool HoldsBackTheRepeat(const AocScript& script);
    [[nodiscard]] bool ProvedAliveWithin(int ticks) const;

    FslabsMcduGateway& mcdu_;
    std::deque<const AocScript*> queue_;
    std::map<const AocScript*, Completion> completions_;
    std::optional<Run> run_;
    std::optional<long long> lastProofTick_;
    std::string lastAbortReport_;
    long long ticks_ = 0;
    long long nextProbeTick_ = 0;
    long long nextInitCheckTick_ = 0;
    bool initDone_ = false;
    bool wasAlive_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSAOC_H
