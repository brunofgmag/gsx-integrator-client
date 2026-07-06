#include "GsxRemoteStateReducer.h"

#include <utility>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
    std::string Str(const QJsonValue& v) { return v.toString().toStdString(); }

    void SetServices(GsxRemoteState& state, const QJsonValue& value)
    {
        state.services.clear();
        for (const QJsonValue& v : value.toArray())
        {
            const QJsonObject& o = v.toObject();

            GsxRemoteService svc;
            svc.id = Str(o.value("id"));
            svc.displayName = Str(o.value("displayName"));
            svc.state = Str(o.value("state"));
            svc.stateRaw = o.value("stateRaw").toInt();
            svc.stateText = Str(o.value("stateText"));
            svc.statusText = Str(o.value("statusText"));
            svc.progressText = Str(o.value("progressText"));
            svc.canTrigger = o.value("canTrigger").toBool();
            svc.canBypass = o.value("canBypass").toBool();

            state.services.push_back(std::move(svc));
        }
    }

    void SetMenu(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& o = value.toObject();

        state.menu.title = Str(o.value("title"));
        state.menu.subtitle = Str(o.value("subtitle"));
        state.menu.header = Str(o.value("header"));

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

    void SetMessage(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& o = value.toObject();

        state.message.text = Str(o.value("text"));
        state.message.visible = o.value("visible").toBool();
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
    state.sessionState = snapshot.value("state").toInt(state.sessionState);

    if (snapshot.contains("stateText"))
    {
        state.sessionStateText = Str(snapshot.value("stateText"));
    }

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

    if (snapshot.contains("message"))
    {
        SetMessage(state, snapshot.value("message"));
    }

    if (snapshot.contains("simbrief"))
    {
        SetSimBrief(state, snapshot.value("simbrief"));
    }
}

void GsxRemoteStateReducer::ApplyPatch(GsxRemoteState& state, const std::string& path, const QJsonValue& value)
{
    if (path == "/services")
    {
        SetServices(state, value);
    }

    if (path == "/menu")
    {
        SetMenu(state, value);
    }

    if (path == "/menuShown")
    {
        state.menu.shown = value.toBool();
    }

    if (path == "/message")
    {
        SetMessage(state, value);
    }

    if (path == "/simbrief")
    {
        SetSimBrief(state, value);
    }

    if (path == "/state")
    {
        state.sessionState = value.toInt();
    }

    if (path == "/stateText")
    {
        state.sessionStateText = Str(value);
    }
}
