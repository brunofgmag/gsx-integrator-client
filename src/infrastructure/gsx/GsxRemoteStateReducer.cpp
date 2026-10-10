#include "GsxRemoteStateReducer.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
    constexpr std::array<std::string_view, 4> kDiscardedPaths = {
        "/billing", "/message", "/state", "/stateText"
    };

    std::string Str(const QJsonValue& value) { return value.toString().toStdString(); }

    void SetServices(GsxRemoteState& state, const QJsonValue& value)
    {
        state.services.clear();
        for (const QJsonValueConstRef& item : value.toArray())
        {
            const QJsonObject& object = item.toObject();

            GsxRemoteService svc;
            svc.id = Str(object.value("id"));
            svc.stateRaw = object.value("stateRaw").toInt();
            svc.canTrigger = object.value("canTrigger").toBool();

            state.services.push_back(std::move(svc));
        }
    }

    void SetMenu(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& object = value.toObject();

        state.menu.title = Str(object.value("title"));

        state.menu.entries.clear();
        for (const QJsonValueConstRef& entry : object.value("entries").toArray())
        {
            state.menu.entries.push_back(Str(entry));
        }

        state.menu.disabled.clear();
        for (const QJsonValueConstRef& flag : object.value("disabled").toArray())
        {
            state.menu.disabled.push_back(flag.toBool());
        }
    }

    void SetSimBrief(GsxRemoteState& state, const QJsonValue& value)
    {
        const QJsonObject& object = value.toObject();

        state.simbriefStatus = Str(object.value("status"));
        state.simbriefError = Str(object.value("error"));
        state.simbriefGeneration = object.value("gen").toInt();
    }

    void SetOperators(GsxRemoteState& state, const QJsonValue& value)
    {
        state.handlingOperator = Str(value.toObject().value("handling"));
    }

    void SetApronVerdict(GsxRemoteState& state, const QJsonValue& value)
    {
        state.apronVerdict.clear();
        for (const QJsonValueConstRef& verdict : value.toArray())
        {
            state.apronVerdict.push_back(Str(verdict));
        }
    }

    void SetMenuShown(GsxRemoteState& state, const QJsonValue& value)
    {
        state.menu.shown = value.toBool();
    }

    void SetMatchedAircraft(GsxRemoteState& state, const QJsonValue& value)
    {
        state.matchedAircraftTitle = Str(value);
    }

    std::optional<std::string> WholeNumberText(const QJsonValue& number)
    {
        const qint64 whole = number.toInteger();
        if (static_cast<double>(whole) != number.toDouble())
        {
            return std::nullopt;
        }

        return std::to_string(whole);
    }

    std::optional<std::string> SidText(const QJsonValue& sid)
    {
        if (sid.isDouble())
        {
            return WholeNumberText(sid);
        }

        if (sid.isString() && !sid.toString().isEmpty())
        {
            return Str(sid);
        }

        return std::nullopt;
    }

    void SetCouatlId(GsxRemoteState& state, const QJsonValue& value)
    {
        if (const std::optional<std::string> sid = SidText(value.toObject().value("sid")))
        {
            state.couatlId = *sid;
        }
    }

    void SetAirportIcao(GsxRemoteState& state, const QJsonValue& value)
    {
        state.airportIcao = Str(value.toObject().value("icao"));
    }

    void SetParkingName(GsxRemoteState& state, const QJsonValue& value)
    {
        state.parkingName = Str(value);
    }

    struct StateField
    {
        std::string_view key;
        void (*apply)(GsxRemoteState&, const QJsonValue&);
    };

    constexpr std::array<StateField, 10> kStateFields = {{
        {.key = "services", .apply = SetServices},
        {.key = "menu", .apply = SetMenu},
        {.key = "menuShown", .apply = SetMenuShown},
        {.key = "simbrief", .apply = SetSimBrief},
        {.key = "operators", .apply = SetOperators},
        {.key = "gateProperties", .apply = SetApronVerdict},
        {.key = "aircraft", .apply = SetMatchedAircraft},
        {.key = "startup", .apply = SetCouatlId},
        {.key = "airport", .apply = SetAirportIcao},
        {.key = "parking", .apply = SetParkingName}
    }};

    QLatin1StringView JsonKey(const std::string_view key)
    {
        return {key.data(), static_cast<qsizetype>(key.size())};
    }
}

void GsxRemoteStateReducer::ApplySnapshot(GsxRemoteState& state, const QJsonObject& snapshot)
{
    for (const StateField& field : kStateFields)
    {
        if (snapshot.contains(JsonKey(field.key)))
        {
            field.apply(state, snapshot.value(JsonKey(field.key)));
        }
    }

    state.synced = true;
}

GsxPatchOutcome GsxRemoteStateReducer::ApplyPatch(GsxRemoteState& state, const std::string& path,
                                                  const QJsonValue& value)
{
    if (path.starts_with('/'))
    {
        const std::string_view key = std::string_view(path).substr(1);
        const auto field = std::ranges::find(kStateFields, key, &StateField::key);
        if (field != kStateFields.end())
        {
            field->apply(state, value);

            return GsxPatchOutcome::Applied;
        }
    }

    if (std::ranges::find(kDiscardedPaths, path) != kDiscardedPaths.end())
    {
        return GsxPatchOutcome::Discarded;
    }

    return GsxPatchOutcome::Unknown;
}

void GsxRemoteStateReducer::ApplyConnection(GsxRemoteState& state, const bool connected)
{
    if (connected)
    {
        state = GsxRemoteState{};
    }

    state.connected = connected;
}
