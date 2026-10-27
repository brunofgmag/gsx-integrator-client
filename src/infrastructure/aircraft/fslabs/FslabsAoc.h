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
    FslabsAoc(FslabsMcduGateway& rightMcdu, const FslabsMcduDisplay& leftMcdu);

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
    void RememberTheStop();
    void RememberTheInitCheck();
    void ReadTheInit();
    void Abort(const FslabsMcduScreen& screen);
    void ReportAbort(const FslabsMcduScreen& screen);
    void LeaveTheMenu();
    void EndRun();
    void ReportLifeChange();
    [[nodiscard]] bool McduShowsAScreen() const;
    [[nodiscard]] bool InitIsPending(bool initIsAwaited) const;
    [[nodiscard]] bool InitCheckIsDue(bool initIsAwaited) const;
    [[nodiscard]] bool LifeIsWanted(bool initIsAwaited, bool powerIsAsked) const;
    [[nodiscard]] bool ProbeIsDue(bool initIsAwaited, bool powerIsAsked) const;
    [[nodiscard]] bool IsRunningOrQueued(const AocScript& script) const;
    [[nodiscard]] bool HasGivenUp(const AocScript& script) const;
    [[nodiscard]] bool HoldsBackTheRepeat(const AocScript& script);

    FslabsMcduGateway& rightMcdu_;
    const FslabsMcduDisplay& leftMcdu_;
    std::deque<const AocScript*> queue_;
    std::map<const AocScript*, Completion> completions_;
    std::map<const AocScript*, int> stops_;
    std::optional<Run> run_;
    std::string lastAbortReport_;
    long long ticks_ = 0;
    long long nextProbeTick_ = 0;
    bool aliveIsProved_ = false;
    bool initCheckRan_ = false;
    bool initDone_ = false;
    bool wasAlive_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSAOC_H
