#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDRESUMPTION_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDRESUMPTION_H

#include <optional>

#include "TurnaroundKeyJudgement.h"
#include "model/TurnaroundDocument.h"
#include "model/TurnaroundHold.h"
#include "ports/TurnaroundCheckpointStore.h"

class TurnaroundResumption
{
public:
    struct LiveFacts
    {
        TurnaroundKey key;
        bool readingsArrived = false;
        bool aircraftReachable = false;
    };

    struct Restoration
    {
        const TurnaroundDocument* document = nullptr;
        bool gsxRestartedSinceSave = false;
    };

    explicit TurnaroundResumption(TurnaroundCheckpointStore* store = nullptr);

    [[nodiscard]] TurnaroundHold Hold() const;
    [[nodiscard]] bool IsHolding() const { return Hold() != TurnaroundHold::None; }
    [[nodiscard]] bool HasPending() const { return pending_.has_value(); }

    void Load();
    void Forget();
    void Discard();
    void RetryPendingDiscard();
    void AnswerResume();
    [[nodiscard]] std::optional<Restoration> Advance(const LiveFacts& live);
    void Settle();
    void Save(const TurnaroundDocument& document);

private:
    enum class Stage
    {
        Idle,
        Judging,
        Deciding,
        Restoring,
    };

    void Judge(const TurnaroundKey& live);
    void EnterJudgedStage(bool couatlDiffers);
    void NoteCouatlChangedWhileRestoring();
    [[nodiscard]] bool TryErase();
    void NoteWriteFailure();

    TurnaroundCheckpointStore* store_;
    std::optional<TurnaroundDocument> pending_;
    std::optional<TurnaroundDocument> lastWritten_;
    Stage stage_ = Stage::Idle;
    TurnaroundHold restoringHold_ = TurnaroundHold::AwaitingGsxReadings;
    bool couatlDiffers_ = false;
    bool pilotResumed_ = false;
    bool discardPending_ = false;
    bool discardFailureLogged_ = false;
    bool writeFailureLogged_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDRESUMPTION_H
