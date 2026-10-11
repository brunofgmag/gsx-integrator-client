#include "TurnaroundResumption.h"

#include <utility>

#include "../infrastructure/logging/LogMacros.h"

namespace
{
    std::optional<TurnaroundPhase> PhaseOf(const TurnaroundDocument& document)
    {
        if (!document.checkpoint)
        {
            return std::nullopt;
        }

        return document.checkpoint->phase;
    }

    bool HasAnythingToRead(const TurnaroundDocument& document)
    {
        return document.checkpoint.has_value() || document.repositioned;
    }
}

TurnaroundResumption::TurnaroundResumption(TurnaroundCheckpointStore* store)
    : store_(store)
{
}

TurnaroundHold TurnaroundResumption::Hold() const
{
    switch (stage_)
    {
    case Stage::Judging:
        return TurnaroundHold::JudgingSavedTurnaround;
    case Stage::Deciding:
        return TurnaroundHold::AwaitingResumeDecision;
    case Stage::Restoring:
        return restoringHold_;
    case Stage::Idle:
        break;
    }

    return TurnaroundHold::None;
}

void TurnaroundResumption::Load()
{
    Forget();

    if (store_ == nullptr || (discardPending_ && !TryErase()))
    {
        return;
    }

    pending_ = store_->Read();
    if (pending_ && !HasAnythingToRead(*pending_))
    {
        pending_.reset();
    }

    if (pending_)
    {
        stage_ = Stage::Judging;
    }
}

void TurnaroundResumption::Forget()
{
    pending_.reset();
    releasedRepositioned_ = false;
    stage_ = Stage::Idle;
    couatlDiffers_ = false;
    pilotResumed_ = false;
}

void TurnaroundResumption::Discard()
{
    Forget();
    lastWritten_.reset();

    if (store_ != nullptr)
    {
        discardPending_ = !TryErase();
    }
}

void TurnaroundResumption::RetryPendingDiscard()
{
    if (discardPending_)
    {
        discardPending_ = !TryErase();
    }
}

bool TurnaroundResumption::TryErase()
{
    if (store_->Erase())
    {
        discardFailureLogged_ = false;

        return true;
    }

    if (!discardFailureLogged_)
    {
        discardFailureLogged_ = true;
        LOG_WARN("The saved turnaround could not be deleted. It will not be restored, and deleting it is retried every second.");
    }

    return false;
}

void TurnaroundResumption::AnswerResume()
{
    if (stage_ != Stage::Deciding)
    {
        return;
    }

    stage_ = Stage::Restoring;
    pilotResumed_ = true;
    restoringHold_ = TurnaroundHold::AwaitingGsxReadings;
}

TurnaroundResumption::Advancement TurnaroundResumption::Advance(const LiveFacts& live)
{
    Judge(live.key);

    Advancement advancement;
    advancement.repositioned = std::exchange(releasedRepositioned_, false);
    if (!pending_ || stage_ != Stage::Restoring)
    {
        return advancement;
    }

    restoringHold_ = live.readingsArrived ? TurnaroundHold::AwaitingAircraft : TurnaroundHold::AwaitingGsxReadings;
    if (!live.readingsArrived || !live.aircraftReachable)
    {
        return advancement;
    }

    advancement.restoration = Restoration{.document = &*pending_, .gsxRestartedSinceSave = couatlDiffers_};
    advancement.repositioned = pending_->repositioned && !couatlDiffers_;

    return advancement;
}

void TurnaroundResumption::Judge(const TurnaroundKey& live)
{
    if (!pending_)
    {
        return;
    }

    const KeyVerdict verdict = TurnaroundKeyJudgement::Judge(pending_->key, live, PhaseOf(*pending_));
    if (verdict == KeyVerdict::NotYetJudgeable)
    {
        return;
    }

    if (verdict == KeyVerdict::Different)
    {
        LOG_INFO("The saved turnaround belongs to another aircraft or stand. Discarding it.");
        Discard();

        return;
    }

    if (!pending_->checkpoint)
    {
        const bool repositioned = verdict == KeyVerdict::Same;
        Forget();
        releasedRepositioned_ = repositioned;

        return;
    }

    const bool couatlDiffers = verdict == KeyVerdict::OnlyCouatlDiffers;
    if (stage_ == Stage::Judging)
    {
        EnterJudgedStage(couatlDiffers);
    }
    else if (stage_ == Stage::Restoring && couatlDiffers)
    {
        NoteCouatlChangedWhileRestoring();
    }
}

void TurnaroundResumption::EnterJudgedStage(const bool couatlDiffers)
{
    couatlDiffers_ = couatlDiffers;
    restoringHold_ = TurnaroundHold::AwaitingGsxReadings;

    if (couatlDiffers_)
    {
        stage_ = Stage::Deciding;
        LOG_INFO("The saved turnaround is from another GSX run. Waiting for the pilot to resume or restart it.");

        return;
    }

    stage_ = Stage::Restoring;
    LOG_INFO("The saved turnaround matches this flight. Waiting for the readings before resuming it.");
}

void TurnaroundResumption::NoteCouatlChangedWhileRestoring()
{
    couatlDiffers_ = true;

    if (pilotResumed_)
    {
        return;
    }

    stage_ = Stage::Deciding;
    LOG_INFO("GSX restarted while the saved turnaround waited for its readings. Waiting for the pilot to resume or restart it.");
}

void TurnaroundResumption::Settle()
{
    lastWritten_ = pending_;
    Forget();
}

void TurnaroundResumption::Save(const TurnaroundDocument& document)
{
    if (store_ == nullptr || lastWritten_ == document)
    {
        return;
    }

    if (!store_->Write(document))
    {
        NoteWriteFailure();

        return;
    }

    writeFailureLogged_ = false;
    discardPending_ = false;
    discardFailureLogged_ = false;
    lastWritten_ = document;
    Forget();
}

void TurnaroundResumption::NoteWriteFailure()
{
    if (writeFailureLogged_)
    {
        return;
    }

    writeFailureLogged_ = true;
    LOG_WARN("The turnaround could not be saved. Saving it is retried every second.");
}
