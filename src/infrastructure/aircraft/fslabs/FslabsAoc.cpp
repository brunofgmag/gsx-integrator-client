#include "FslabsAoc.h"

#include <algorithm>
#include <initializer_list>
#include <vector>
#include "../../logging/LogMacros.h"

namespace
{
    enum class ScriptKind : std::uint8_t { Probe, InitCheck, Request };

    enum class CellRule : std::uint8_t { Shows, DoesNotShow };

    struct Cell
    {
        int row;
        int column;
        int glyph;
        CellRule rule = CellRule::Shows;
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
    constexpr int kDepartureRow = 4;
    constexpr int kDepartureFirstColumn = 0;
    constexpr int kDepartureLastColumn = 3;
    constexpr int kEmptySquareGlyph = 31;
    constexpr Cell kBoardNowChecked{6, 0, kEmptySquareGlyph, CellRule::DoesNotShow};

    constexpr auto kSentBackToTheMenu = "was sent back to the MCDU MENU";
    constexpr auto kLeftOnItsPage = "was left on that page";

    constexpr int kProbeIntervalTicks = 10;
    constexpr int kRepeatHoldTicks = 60;
    constexpr int kStopsBeforeGivingUp = 3;

    std::vector<Step> ThroughTheAocMenu(const std::initializer_list<Step> tail)
    {
        std::vector<Step> steps{{kMenuKey, kMcduMenuTitle},
                                {kAtsuKey, kAtsuDatalinkTitle},
                                {kAocMenuKey, kAocMenuTitle}};
        steps.insert(steps.end(), tail.begin(), tail.end());
        steps.push_back({kMenuKey, kMcduMenuTitle});

        return steps;
    }

    bool TitleContains(const std::string& title, const char* expected)
    {
        return expected == nullptr || title.find(expected) != std::string::npos;
    }

    bool TitleMatches(const char* expected, const FslabsMcduScreen& screen)
    {
        return TitleContains(screen.Row(kTitleRow), expected);
    }

    bool CellMatches(const std::optional<Cell>& cell, const FslabsMcduScreen& screen)
    {
        if (!cell.has_value())
        {
            return true;
        }

        const bool showsTheGlyph = screen.GlyphAt(cell->row, cell->column) == cell->glyph;
        const bool wantsTheGlyph = cell->rule == CellRule::Shows;

        return showsTheGlyph == wantsTheGlyph;
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

    bool ReturnsToTheMenuWhenAborted(const ScriptKind kind)
    {
        return kind == ScriptKind::Request;
    }

    bool KeyChangedNothing(const Step& step, const std::string& titleBeforePress, const FslabsMcduScreen& screen)
    {
        return !TitleContains(titleBeforePress, step.title) && screen.Row(kTitleRow) == titleBeforePress;
    }

    bool ReportsTheAbort(const ScriptKind kind, const bool keyChangedNothing)
    {
        return kind == ScriptKind::Request || (kind == ScriptKind::InitCheck && !keyChangedNothing);
    }

    bool DepartureIsFilled(const FslabsMcduScreen& screen)
    {
        return screen.IsFilledBetween(kDepartureRow, kDepartureFirstColumn, kDepartureLastColumn);
    }

    bool ShowsTheInit(const FslabsMcduDisplay& mcdu)
    {
        return mcdu.IsAvailable() && TitleMatches(kInitReviewTitle, mcdu.Screen()) && DepartureIsFilled(mcdu.Screen());
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

FslabsAoc::FslabsAoc(FslabsMcduGateway& rightMcdu, const FslabsMcduDisplay& leftMcdu)
    : rightMcdu_(rightMcdu),
      leftMcdu_(leftMcdu)
{
}

void FslabsAoc::Advance(const bool initIsAwaited, const bool powerIsAsked)
{
    ++ticks_;
    ReadTheInit();

    if (!McduShowsAScreen())
    {
        aliveIsProved_ = false;
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
    return McduShowsAScreen() && aliveIsProved_;
}

bool FslabsAoc::IsInitDone() const
{
    return initDone_;
}

bool FslabsAoc::Request(const AocRequest request)
{
    const AocScript& script = ScriptFor(request);
    if (IsRunningOrQueued(script) || HasGivenUp(script) || HoldsBackTheRepeat(script))
    {
        return false;
    }

    queue_.push_back(&script);

    return true;
}

void FslabsAoc::ResetForNewTurnaround()
{
    initDone_ = false;
    initCheckRan_ = false;
    queue_.clear();
    completions_.clear();
    stops_.clear();

    if (run_.has_value())
    {
        LeaveTheMenu();
    }
}

bool FslabsAoc::McduShowsAScreen() const
{
    return rightMcdu_.IsAvailable() && !rightMcdu_.Screen().IsBlank();
}

bool FslabsAoc::InitIsPending(const bool initIsAwaited) const
{
    return initIsAwaited && !initDone_;
}

bool FslabsAoc::InitCheckIsDue(const bool initIsAwaited) const
{
    return InitIsPending(initIsAwaited) && !initCheckRan_;
}

bool FslabsAoc::LifeIsWanted(const bool initIsAwaited, const bool powerIsAsked) const
{
    return powerIsAsked || !queue_.empty() || InitIsPending(initIsAwaited);
}

bool FslabsAoc::ProbeIsDue(const bool initIsAwaited, const bool powerIsAsked) const
{
    return LifeIsWanted(initIsAwaited, powerIsAsked) && !aliveIsProved_ && ticks_ >= nextProbeTick_;
}

bool FslabsAoc::IsRunningOrQueued(const AocScript& script) const
{
    const bool isRunning = run_.has_value() && run_->script == &script;
    const bool isQueued = std::ranges::find(queue_, &script) != queue_.end();

    return isRunning || isQueued;
}

bool FslabsAoc::HasGivenUp(const AocScript& script) const
{
    const auto stops = stops_.find(&script);

    return stops != stops_.end() && stops->second >= kStopsBeforeGivingUp;
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
        Start(InitCheckIsDue(initIsAwaited) ? kInitCheckScript : kProbeScript);

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

    if (InitCheckIsDue(initIsAwaited))
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
    if (rightMcdu_.ScreenSerial() <= run_->serialAtPress)
    {
        return;
    }

    const FslabsMcduScreen& screen = rightMcdu_.Screen();
    const Step& step = run_->script->steps[run_->step];
    if (!Matches(step, screen))
    {
        Abort(screen);

        return;
    }

    if (ChangedThePage(step, run_->titleBeforePress, screen))
    {
        aliveIsProved_ = true;
    }

    ++run_->step;
    PressNext();
}

void FslabsAoc::PressNext()
{
    const std::vector<Step>& steps = run_->script->steps;
    const FslabsMcduScreen& screen = rightMcdu_.Screen();
    while (run_->step < steps.size() && AlreadyDone(steps[run_->step], screen))
    {
        ++run_->step;
    }

    if (run_->step == steps.size())
    {
        lastAbortReport_.clear();
        RememberTheCompletion();
        RememberTheInitCheck();
        EndRun();

        return;
    }

    run_->titleBeforePress = screen.Row(kTitleRow);
    rightMcdu_.Press(steps[run_->step].key);
    run_->serialAtPress = rightMcdu_.ScreenSerial();
}

void FslabsAoc::RememberTheCompletion()
{
    if (run_->script->kind == ScriptKind::Request)
    {
        completions_[run_->script] = Completion{.tick = ticks_};
        stops_.erase(run_->script);
    }
}

void FslabsAoc::RememberTheStop()
{
    if (run_->script->kind != ScriptKind::Request)
    {
        return;
    }

    const int stops = ++stops_[run_->script];
    if (stops == kStopsBeforeGivingUp)
    {
        LOG_WARN("FSLabs AOC: the %s stopped three times in a row; ask for it on the AOC yourself",
                 run_->script->name);
    }
}

void FslabsAoc::RememberTheInitCheck()
{
    if (run_->script->kind == ScriptKind::InitCheck && aliveIsProved_)
    {
        initCheckRan_ = true;
    }
}

void FslabsAoc::ReadTheInit()
{
    if (ShowsTheInit(rightMcdu_) || ShowsTheInit(leftMcdu_))
    {
        initDone_ = true;
    }
}

void FslabsAoc::Abort(const FslabsMcduScreen& screen)
{
    const bool keyChangedNothing = KeyChangedNothing(run_->script->steps[run_->step], run_->titleBeforePress, screen);
    if (keyChangedNothing)
    {
        aliveIsProved_ = false;
    }

    const ScriptKind kind = run_->script->kind;
    if (ReportsTheAbort(kind, keyChangedNothing))
    {
        ReportAbort(screen);
    }

    RememberTheStop();
    RememberTheInitCheck();

    if (ReturnsToTheMenuWhenAborted(kind))
    {
        LeaveTheMenu();

        return;
    }

    EndRun();
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

    const char* const outcome = ReturnsToTheMenuWhenAborted(run_->script->kind) ? kSentBackToTheMenu : kLeftOnItsPage;

    LOG_WARN("FSLabs AOC: the %s stopped at step %d, the right MCDU shows '%s' and %s",
             run_->script->name, static_cast<int>(run_->step), shown.c_str(), outcome);
}

void FslabsAoc::LeaveTheMenu()
{
    rightMcdu_.Press(kMenuKey);
    EndRun();
}

void FslabsAoc::EndRun()
{
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
