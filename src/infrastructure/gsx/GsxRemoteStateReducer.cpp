#include "GsxRemoteStateReducer.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
    constexpr std::array<std::string_view, 5> kDiscardedPaths = {
        "/billing", "/operators", "/message", "/state", "/stateText"
    };

    std::string Str(const QJsonValue& v) { return v.toString().toStdString(); }

    void SetServices(GsxRemoteState& state, const QJsonValue& value)
    {
        state.services.clear();
        for (const QJsonValue& v : value.toArray())
        {
            const QJsonObject& o = v.toObject();

            GsxRemoteService svc;
            svc.id = Str(o.value("id"));
            svc.stateRaw = o.value("stateRaw").toInt();
            svc.canTrigger = o.value("canTrigger").toBool();

            state.services.push_back(std::move(svc));
        }
    }

    void SetMenu(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& o = value.toObject();

        state.menu.title = Str(o.value("title"));

        state.menu.entries.clear();
        for (const QJsonValue& v : o.value("entries").toArray())
        {
            state.menu.entries.push_back(Str(v));
        }

        state.menu.disabled.clear();
        for (const QJsonValue& v : o.value("disabled").toArray())
        {
            state.menu.disabled.push_back(v.toBool());
        }
    }

    void SetSimBrief(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& o = value.toObject();

        state.simbriefStatus = Str(o.value("status"));
        state.simbriefError = Str(o.value("error"));
    }
}

void GsxRemoteStateReducer::ApplySnapshot(GsxRemoteState& state, const QJsonObject& snapshot)
{
    if (snapshot.contains("services"))
    {
        SetServices(state, snapshot.value("services"));
    }

    if (snapshot.contains("menu"))
    {
        SetMenu(state, snapshot.value("menu"));
    }

    if (snapshot.contains("menuShown"))
    {
        state.menu.shown = snapshot.value("menuShown").toBool();
    }

    if (snapshot.contains("simbrief"))
    {
        SetSimBrief(state, snapshot.value("simbrief"));
    }
}

GsxPatchOutcome GsxRemoteStateReducer::ApplyPatch(GsxRemoteState& state, const std::string& path,
                                                  const QJsonValue& value)
{
    if (path == "/services")
    {
        SetServices(state, value);
    }
    else if (path == "/menu")
    {
        SetMenu(state, value);
    }
    else if (path == "/menuShown")
    {
        state.menu.shown = value.toBool();
    }
    else if (path == "/simbrief")
    {
        SetSimBrief(state, value);
    }
    else if (std::ranges::find(kDiscardedPaths, path) != kDiscardedPaths.end())
    {
        return GsxPatchOutcome::Discarded;
    }
    else
    {
        return GsxPatchOutcome::Unknown;
    }

    return GsxPatchOutcome::Applied;
}
