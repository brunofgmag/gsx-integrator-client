#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTXML_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTXML_H

#include <functional>
#include <optional>
#include <vector>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>
#include "ProtonPrefix.h"

inline constexpr quint16 kStaticSimConnectPort = 5111;

enum class SimConnectXmlStatus
{
    Present,
    Added,
    Created,
    SimulatorRunning,
    Failed
};

struct SimConnectXmlResult
{
    SimConnectXmlStatus status;
    quint16 port;
};

struct SimConnectXmlTarget
{
    QString label;
    QString path;
    QString processName;
};

[[nodiscard]] std::optional<quint16> ParseSimConnectPort(const QByteArray& simConnectXml);
[[nodiscard]] std::optional<quint16> ReadSimConnectPort(const QString& simConnectXmlPath);
[[nodiscard]] SimConnectXmlResult EnsureSimConnectPort(const QString& simConnectXmlPath,
                                                       const std::function<bool()>& isSimulatorRunning);
[[nodiscard]] std::vector<SimConnectXmlTarget> SimConnectXmlTargets(const std::vector<ProtonPrefix>& prefixes);
[[nodiscard]] std::vector<SimConnectXmlTarget> SimConnectXmlTargets(const QString& machineHome);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTXML_H
