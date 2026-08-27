#include "EfbCommandReceiver.h"

#include <array>
#include <optional>
#include <utility>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "../logging/LogMacros.h"
#include "../../domain/turnaround/TurnaroundPhase.h"
#include "../../viewmodel/OperationsViewModel.h"

namespace
{
    constexpr auto kPilotTouch = QLatin1String("pilotTouch");

    using Touch = void (OperationsViewModel::*)();

    constexpr std::array<std::pair<QLatin1String, Touch>, 4> kTouches{{
        {QLatin1String("startFlow"), &OperationsViewModel::startFlow},
        {QLatin1String("startLoading"), &OperationsViewModel::startLoading},
        {QLatin1String("restartFlow"), &OperationsViewModel::restartFlow},
        {QLatin1String("reloadSimbrief"), &OperationsViewModel::reloadSimbrief},
    }};

    std::optional<QJsonObject> CommandObject(const std::string& payload)
    {
        const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(payload));
        if (!document.isObject())
        {
            LOG_WARN("EFB app: dropping a command payload that is not an object");

            return std::nullopt;
        }

        const QJsonObject object = document.object();
        if (!object.value(QLatin1String("command")).isString())
        {
            LOG_WARN("EFB app: dropping a command payload without a command name");

            return std::nullopt;
        }

        return object;
    }

    std::optional<TurnaroundPhase> StampedPhase(const QJsonObject& command)
    {
        const QJsonValue phase = command.value(QLatin1String("phase"));
        if (!phase.isDouble())
        {
            LOG_WARN("EFB app: dropping a pilot touch that carries no phase stamp");

            return std::nullopt;
        }

        const int index = phase.toInt(-1);
        if (index < 0 || index >= static_cast<int>(TurnaroundPhase::Count))
        {
            LOG_WARN("EFB app: dropping a pilot touch stamped with the unknown phase %d", index);

            return std::nullopt;
        }

        return static_cast<TurnaroundPhase>(index);
    }
}

EfbCommandReceiver::EfbCommandReceiver(CommBusBridgeGateway* bridge, OperationsViewModel* view)
    : bridge_(bridge), view_(view)
{
}

void EfbCommandReceiver::Setup()
{
    bridge_->Subscribe(EfbCommBus::kCommandChannel, CommBusFlag::kJs,
                       [this](const std::string& payload) { Accept(payload); });
}

void EfbCommandReceiver::Accept(const std::string& payload) const
{
    const std::optional<QJsonObject> command = CommandObject(payload);
    if (!command.has_value())
    {
        return;
    }

    const QString name = command->value(QLatin1String("command")).toString();

    if (name == kPilotTouch)
    {
        const std::optional<TurnaroundPhase> stamped = StampedPhase(*command);
        if (stamped.has_value())
        {
            view_->AcceptPilotTouch(*stamped);
        }

        return;
    }

    for (const auto& [known, touch] : kTouches)
    {
        if (name == known)
        {
            (view_->*touch)();

            return;
        }
    }

    LOG_WARN("EFB app: dropping the unknown command '%s'", qUtf8Printable(name));
}
