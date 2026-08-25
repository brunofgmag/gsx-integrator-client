#include "EfbCommandReceiver.h"

#include <array>
#include <optional>
#include <utility>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "../logging/LogMacros.h"
#include "../../viewmodel/OperationsViewModel.h"

namespace
{
    using Touch = void (OperationsViewModel::*)();

    constexpr std::array<std::pair<QLatin1String, Touch>, 4> kTouches{{
        {QLatin1String("startFlow"), &OperationsViewModel::startFlow},
        {QLatin1String("startLoading"), &OperationsViewModel::startLoading},
        {QLatin1String("restartFlow"), &OperationsViewModel::restartFlow},
        {QLatin1String("reloadSimbrief"), &OperationsViewModel::reloadSimbrief},
    }};

    std::optional<QString> CommandName(const std::string& payload)
    {
        const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(payload));
        if (!document.isObject())
        {
            LOG_WARN("EFB app: dropping a command payload that is not an object");

            return std::nullopt;
        }

        const QJsonValue command = document.object().value(QLatin1String("command"));
        if (!command.isString())
        {
            LOG_WARN("EFB app: dropping a command payload without a command name");

            return std::nullopt;
        }

        return command.toString();
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
    const std::optional<QString> name = CommandName(payload);
    if (!name.has_value())
    {
        return;
    }

    for (const auto& [known, touch] : kTouches)
    {
        if (*name == known)
        {
            (view_->*touch)();

            return;
        }
    }

    LOG_WARN("EFB app: dropping the unknown command '%s'", qUtf8Printable(*name));
}
