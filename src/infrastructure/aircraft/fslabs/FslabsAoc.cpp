#include "FslabsAoc.h"

#include <algorithm>
#include <initializer_list>
#include <vector>
#include "../../logging/LogMacros.h"

namespace
{
    enum class ScriptKind : std::uint8_t { Probe, InitCheck, Request };

    struct Cell
    {
        int row;
        int column;
        int glyph;
    };

    struct Step
    {
        int key;
        const char* title;
        std::optional<Cell> cell = std::nullopt;
    };

    constexpr int kMenuKey = 96;
    constexpr int kClearKey = 8;
    constexpr int kInitKey = 112;
    constexpr int kOfpDataKey = 113;
    constexpr int kPushbackKey = 113;
    constexpr int kBoardNowKey = 114;
    constexpr int kAtsuKey = 117;
    constexpr int kAocMenuKey = 121;
    constexpr int kStdKey = 122;
    constexpr int kBoardingKey = 123;
    constexpr int kDepartureKey = 124;
    constexpr int kSendKey = 125;
    constexpr int kBoardingRequestKey = 125;
    constexpr int kPushNowKey = 125;

    constexpr auto kMcduMenuTitle = "MCDU MENU";
    constexpr auto kAtsuDatalinkTitle = "ATSU DATALINK";
    constexpr auto kAocMenuTitle = "AOC MENU";
    constexpr auto kInitReviewTitle = "INIT/REVIEW";
    constexpr auto kOfpDataTitle = "OFP DATA";
    constexpr auto kBoardingFirstPageTitle = "BOARDING     1/2";
    constexpr auto kBoardingSecondPageTitle = "BOARDING     2/2";
    constexpr auto kDepartureMenuTitle = "DEPARTURE MENU";
    constexpr auto kPushbackPageTitle = "GSX PUSH BACK";

    constexpr int kTitleRow = 0;
    constexpr int kFlightNumberRow = 2;
    constexpr int kFlightNumberFirstColumn = 0;
    constexpr int kFlightNumberLastColumn = 9;
    constexpr Cell kBoardNowChecked{6, 0, 30};

    constexpr int kProbeAfterTicks = 10;
    constexpr int kProbeIntervalTicks = 10;
    constexpr int kAliveForTicks = 19;
    constexpr int kInitCheckIntervalTicks = 10;
    constexpr int kRepeatHoldTicks = 60;

    std::vector<Step> ThroughTheAocMenu(const std::initializer_list<Step> tail)
    {
        std::vector<Step> steps{{kMenuKey, kMcduMenuTitle},
                                {kAtsuKey, kAtsuDatalinkTitle},
                                {kAocMenuKey, kAocMenuTitle}};
        steps.insert(steps.end(), tail.begin(), tail.end());
        steps.push_back({kMenuKey, kMcduMenuTitle});

        return steps;
    }

    bool TitleMatches(const char* expected, const FslabsMcduScreen& screen)
    {
        return expected == nullptr || screen.Row(kTitleRow).find(expected) != std::string::npos;
    }

    bool CellMatches(const std::optional<Cell>& cell, const FslabsMcduScreen& screen)
    {
        return !cell.has_value() || screen.GlyphAt(cell->row, cell->column) == cell->glyph;
    }

    bool Matches(const Step& step, const FslabsMcduScreen& screen)
    {
        return TitleMatches(step.title, screen) && CellMatches(step.cell, screen);
    }

    bool AlreadyDone(const Step& step, const FslabsMcduScreen& screen)
    {
        return step.cell.has_value() && CellMatches(step.cell, screen);
    }

    bool ChangedThePage(const Step& step, const std::string& titleBeforePress, const FslabsMcduScreen& screen)
    {
        return step.title != nullptr && screen.Row(kTitleRow) != titleBeforePress;
    }

    bool FlightNumberIsFilled(const FslabsMcduScreen& screen)
    {
        return !screen.IsBlankBetween(kFlightNumberRow, kFlightNumberFirstColumn, kFlightNumberLastColumn);
    }

    std::string Trimmed(const std::string& text)
    {
        const std::size_t first = text.find_first_not_of(' ');
        if (first == std::string::npos)
        {
            return {};
        }

        return text.substr(first, text.find_last_not_of(' ') - first + 1);
    }
}

struct AocScript
{
    const char* name;
    ScriptKind kind;
    std::vector<Step> steps;
};

namespace
{
    const AocScript kProbeScript{"probe", ScriptKind::Probe,
                                 {{kMenuKey, kMcduMenuTitle},
                                  {kAtsuKey, kAtsuDatalinkTitle},
                                  {kMenuKey, kMcduMenuTitle}}};

    const AocScript kInitCheckScript{"INIT check", ScriptKind::InitCheck,
                                     ThroughTheAocMenu({{kInitKey, kInitReviewTitle}})};

    const AocScript kSendOfpScript{"OFP send", ScriptKind::Request,
                                   ThroughTheAocMenu({{kOfpDataKey, kOfpDataTitle},
                                                      {kClearKey, kOfpDataTitle},
                                                      {kStdKey, kOfpDataTitle},
                                                      {kSendKey, nullptr}})};

    const AocScript kBoardScript{"boarding request", ScriptKind::Request,
                                 ThroughTheAocMenu({{kBoardingKey, kBoardingFirstPageTitle},
                                                    {kBoardingRequestKey, kBoardingSecondPageTitle},
                                                    {kBoardNowKey, kBoardingSecondPageTitle, kBoardNowChecked},
                                                    {kBoardingRequestKey, nullptr}})};

    const AocScript kPushbackScript{"pushback request", ScriptKind::Request,
                                    ThroughTheAocMenu({{kDepartureKey, kDepartureMenuTitle},
                                                       {kPushbackKey, kPushbackPageTitle},
                                                       {kPushNowKey, nullptr}})};

    const AocScript& ScriptFor(const AocRequest request)
    {
        switch (request)
        {
        case AocRequest::SendOfp:
            return kSendOfpScript;
        case AocRequest::Board:
            return kBoardScript;
        default:
            return kPushbackScript;
        }
    }
}

FslabsAoc::FslabsAoc(FslabsMcduGateway& mcdu)
    : mcdu_(mcdu)
{
}

void FslabsAoc::Advance(const bool initIsAwaited, const bool powerIsAsked)
{
    ++ticks_;

    if (!McduShowsAScreen())
    {
        EndRun();
    }
    else if (run_.has_value())
    {
        ContinueRun();
    }
    else
    {
        StartNext(initIsAwaited, powerIsAsked);
    }

    ReportLifeChange();
}

bool FslabsAoc::IsAlive() const
{
    return McduShowsAScreen() && ProvedAliveWithin(kAliveForTicks);
}

bool FslabsAoc::IsInitDone() const
{
    return initDone_;
}

bool FslabsAoc::Request(const AocRequest request)
{
    const AocScript& script = ScriptFor(request);
    if (IsRunningOrQueued(script) || HoldsBackTheRepeat(script))
    {
        return false;
    }

    queue_.push_back(&script);

    return true;
}

void FslabsAoc::ResetForNewTurnaround()
{
    initDone_ = false;
    nextInitCheckTick_ = 0;
    queue_.clear();
    completions_.clear();

    if (run_.has_value())
    {
        LeaveTheMenu();
    }
}

bool FslabsAoc::McduShowsAScreen() const
{
    return mcdu_.IsAvailable() && !mcdu_.Screen().IsBlank();
}

bool FslabsAoc::ProvedAliveWithin(const int ticks) const
{
    return lastProofTick_.has_value() && ticks_ - *lastProofTick_ <= ticks;
}

bool FslabsAoc::InitIsPending(const bool initIsAwaited) const
{
    return initIsAwaited && !initDone_;
}

bool FslabsAoc::ProbeIsDue(const bool initIsAwaited, const bool powerIsAsked) const
{
    const bool livenessIsWanted = powerIsAsked || !queue_.empty() || InitIsPending(initIsAwaited);

    return livenessIsWanted && !ProvedAliveWithin(kProbeAfterTicks) && ticks_ >= nextProbeTick_;
}

bool FslabsAoc::IsRunningOrQueued(const AocScript& script) const
{
    const bool isRunning = run_.has_value() && run_->script == &script;
    const bool isQueued = std::ranges::find(queue_, &script) != queue_.end();

    return isRunning || isQueued;
}

bool FslabsAoc::HoldsBackTheRepeat(const AocScript& script)
{
    const auto completion = completions_.find(&script);
    if (completion == completions_.end() || ticks_ - completion->second.tick > kRepeatHoldTicks)
    {
        return false;
    }

    if (!completion->second.reported)
    {
        completion->second.reported = true;
        LOG_INFO("FSLabs AOC: the %s ran a moment ago, the repeat request is not sent", script.name);
    }

    return true;
}

void FslabsAoc::StartNext(const bool initIsAwaited, const bool powerIsAsked)
{
    if (ProbeIsDue(initIsAwaited, powerIsAsked))
    {
        nextProbeTick_ = ticks_ + kProbeIntervalTicks;
        Start(kProbeScript);

        return;
    }

    if (!IsAlive())
    {
        return;
    }

    if (!queue_.empty())
    {
        const AocScript& script = *queue_.front();
        queue_.pop_front();
        Start(script);

        return;
    }

    if (InitIsPending(initIsAwaited) && ticks_ >= nextInitCheckTick_)
    {
        Start(kInitCheckScript);
    }
}

void FslabsAoc::Start(const AocScript& script)
{
    if (script.kind == ScriptKind::Request)
    {
        LOG_INFO("FSLabs AOC: running the %s on the right MCDU", script.name);
    }

    run_ = Run{.script = &script};
    PressNext();
}

void FslabsAoc::ContinueRun()
{
    if (mcdu_.ScreenSerial() <= run_->serialAtPress)
    {
        return;
    }

    const FslabsMcduScreen& screen = mcdu_.Screen();
    const Step& step = run_->script->steps[run_->step];
    if (!Matches(step, screen))
    {
        Abort(screen);

        return;
    }

    RecordArrival(ChangedThePage(step, run_->titleBeforePress, screen), screen);
    ++run_->step;
    PressNext();
}

void FslabsAoc::PressNext()
{
    const std::vector<Step>& steps = run_->script->steps;
    const FslabsMcduScreen& screen = mcdu_.Screen();
    while (run_->step < steps.size() && AlreadyDone(steps[run_->step], screen))
    {
        ++run_->step;
    }

    if (run_->step == steps.size())
    {
        lastAbortReport_.clear();
        RememberTheCompletion();
        EndRun();

        return;
    }

    run_->titleBeforePress = screen.Row(kTitleRow);
    mcdu_.Press(steps[run_->step].key);
    run_->serialAtPress = mcdu_.ScreenSerial();
}

void FslabsAoc::RememberTheCompletion()
{
    if (run_->script->kind == ScriptKind::Request)
    {
        completions_[run_->script] = Completion{.tick = ticks_};
    }
}

void FslabsAoc::RecordArrival(const bool pageChanged, const FslabsMcduScreen& screen)
{
    if (pageChanged)
    {
        lastProofTick_ = ticks_;
    }

    if (TitleMatches(kInitReviewTitle, screen) && FlightNumberIsFilled(screen))
    {
        initDone_ = true;
    }
}

void FslabsAoc::Abort(const FslabsMcduScreen& screen)
{
    if (run_->script->kind != ScriptKind::Probe)
    {
        ReportAbort(screen);
    }

    LeaveTheMenu();
}

void FslabsAoc::ReportAbort(const FslabsMcduScreen& screen)
{
    const std::string shown = Trimmed(screen.Row(kTitleRow));
    const std::string report = std::string(run_->script->name) + "@" + std::to_string(run_->step) + ":" + shown;
    if (report == lastAbortReport_)
    {
        return;
    }

    lastAbortReport_ = report;

    LOG_WARN("FSLabs AOC: the %s stopped at step %d, the right MCDU shows '%s' and was sent back to the MCDU MENU",
             run_->script->name, static_cast<int>(run_->step), shown.c_str());
}

void FslabsAoc::LeaveTheMenu()
{
    mcdu_.Press(kMenuKey);
    EndRun();
}

void FslabsAoc::EndRun()
{
    if (!run_.has_value())
    {
        return;
    }

    if (run_->script->kind == ScriptKind::InitCheck)
    {
        nextInitCheckTick_ = ticks_ + kInitCheckIntervalTicks;
    }

    run_.reset();
}

void FslabsAoc::ReportLifeChange()
{
    const bool alive = IsAlive();
    if (alive == wasAlive_)
    {
        return;
    }

    wasAlive_ = alive;

    if (alive)
    {
        LOG_INFO("FSLabs right MCDU answers its keys: the AOC pages are reachable");

        return;
    }

    LOG_INFO("FSLabs right MCDU is off or unpowered: the AOC requests wait for it");
}
