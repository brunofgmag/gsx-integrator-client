#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FSLABSFIXTURES_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FSLABSFIXTURES_H

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QtLogging>
#include "../src/infrastructure/fslabs/FslabsGroundGateway.h"
#include "../src/infrastructure/fslabs/FslabsMcduClient.h"
#include "../src/infrastructure/fslabs/FslabsMcduGateway.h"

inline QByteArray FslabsFixtureBody(const std::string& name)
{
    QFile file(QStringLiteral(FSLABS_FIXTURES_DIR "/") + QString::fromStdString(name) + QStringLiteral(".json"));
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("missing FSLabs fixture %s", name.c_str());
    }

    return file.readAll();
}

inline FslabsMcduScreen FslabsFixtureScreen(const std::string& name)
{
    const std::optional<FslabsMcduScreen> screen = FslabsMcduClient::ParseDisplayResponse(FslabsFixtureBody(name));
    if (!screen.has_value())
    {
        qFatal("unreadable FSLabs fixture %s", name.c_str());
    }

    return *screen;
}

class FakeFslabsMcdu final : public FslabsMcduGateway
{
public:
    static constexpr int kMenuKey = 96;
    static constexpr auto kBlankPage = "blank";

    explicit FakeFslabsMcdu(std::string startPage = "mcdu-menu")
        : page_(std::move(startPage))
    {
        Route("mcdu-menu", 117, "atsu-datalink");
        Route("atsu-datalink", 121, "aoc-menu");
        Route("aoc-menu", 112, "aoc-init");
        Route("aoc-menu", 113, "ofp-data");
        Route("aoc-menu", 123, "boarding-1");
        Route("aoc-menu", 124, "departure");
        Route("boarding-1", 125, "boarding-2");
        Route("boarding-2", 114, "boarding-2-checked");
        Route("departure", 113, "pushback");
    }

    void Poll() override
    {
        ++polls;
        if (holdsTheScreen)
        {
            return;
        }

        if (reply_.has_value() && reply_->ticksLeft > 0)
        {
            --reply_->ticksLeft;

            return;
        }

        DeliverTheReply();
        reply_ = Reply{page_, pressCount_, DelayOfTheNextReply()};
    }

    [[nodiscard]] bool IsAvailable() const override { return available_; }
    [[nodiscard]] const FslabsMcduScreen& Screen() const override { return screen_; }
    [[nodiscard]] std::uint64_t ScreenSerial() const override { return serial_; }

    void Press(const int key) override
    {
        keys.push_back(key);
        pollsAtKey.push_back(polls);
        ++pressCount_;
        if (!available_ || isFrozen || page_ == kBlankPage)
        {
            return;
        }

        Navigate(key);
    }

    void Route(const std::string& from, const int key, const std::string& to)
    {
        routes_[{from, key}] = to;
    }

    void ForgetThePresses()
    {
        keys.clear();
        pollsAtKey.clear();
    }

    void Show(const std::string& page) { page_ = page; }
    [[nodiscard]] const std::string& Page() const { return page_; }

    [[nodiscard]] int PressesOf(const int key) const
    {
        return static_cast<int>(std::count(keys.begin(), keys.end(), key));
    }

    std::vector<int> keys;
    std::vector<int> pollsAtKey;
    int polls = 0;
    std::deque<int> nextReplyDelays;
    int replyDelayTicks = 0;
    bool reachable = true;
    bool isFrozen = false;
    bool holdsTheScreen = false;

private:
    struct Reply
    {
        std::string page;
        std::uint64_t pressesWhenAsked;
        int ticksLeft;
    };

    int DelayOfTheNextReply()
    {
        if (nextReplyDelays.empty())
        {
            return replyDelayTicks;
        }

        const int delay = nextReplyDelays.front();
        nextReplyDelays.pop_front();

        return delay;
    }

    void DeliverTheReply()
    {
        if (!reply_.has_value())
        {
            return;
        }

        const Reply reply = *reply_;
        reply_.reset();

        available_ = reachable;
        if (!reachable)
        {
            return;
        }

        screen_ = ScreenOf(reply.page);
        if (reply.pressesWhenAsked == pressCount_)
        {
            ++serial_;
        }
    }

    void Navigate(const int key)
    {
        if (key == kMenuKey)
        {
            page_ = "mcdu-menu";

            return;
        }

        const auto route = routes_.find({page_, key});
        if (route != routes_.end())
        {
            page_ = route->second;
        }
    }

    const FslabsMcduScreen& ScreenOf(const std::string& page)
    {
        const auto known = pages_.find(page);
        if (known != pages_.end())
        {
            return known->second;
        }

        return pages_.emplace(page, FslabsFixtureScreen(page)).first->second;
    }

    std::string page_;
    FslabsMcduScreen screen_;
    std::uint64_t serial_ = 0;
    std::uint64_t pressCount_ = 0;
    bool available_ = false;
    std::optional<Reply> reply_;
    std::map<std::string, FslabsMcduScreen> pages_;
    std::map<std::pair<std::string, int>, std::string> routes_;
};

class FakeFslabsGround final : public FslabsGroundGateway
{
public:
    void Poll() override { ++polls; }
    [[nodiscard]] bool IsAvailable() const override { return available; }

    [[nodiscard]] std::optional<FslabsHoldState> HoldState(const FslabsHold hold) const override
    {
        return hold == FslabsHold::Forward ? forwardHold : aftHold;
    }

    void ToggleHold(const FslabsHold hold) override { toggles.push_back(hold); }

    [[nodiscard]] int TogglesOf(const FslabsHold hold) const
    {
        return static_cast<int>(std::count(toggles.begin(), toggles.end(), hold));
    }

    int polls = 0;
    bool available = true;
    std::optional<FslabsHoldState> forwardHold;
    std::optional<FslabsHoldState> aftHold;
    std::vector<FslabsHold> toggles;
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FSLABSFIXTURES_H
