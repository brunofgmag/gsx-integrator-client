#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H

#include <QtCore/QString>

struct SimulatorCandidate
{
    const char* label;
    const char* baseSubPath;
    const char* processName;
    unsigned steamAppId;
};

inline constexpr SimulatorCandidate kSimulatorCandidates[] = {
    {
        "MSFS 2020 (Steam)",
        "AppData/Roaming/Microsoft Flight Simulator",
        "FlightSimulator.exe",
        1250410
    },
    {
        "MSFS 2020 (Microsoft Store)",
        "AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache",
        "FlightSimulator.exe",
        0
    },
    {
        "MSFS 2024 (Steam)",
        "AppData/Roaming/Microsoft Flight Simulator 2024",
        "FlightSimulator2024.exe",
        2537590
    },
    {
        "MSFS 2024 (Microsoft Store)",
        "AppData/Local/Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache",
        "FlightSimulator2024.exe",
        0
    },
};

[[nodiscard]] QString SimulatorBaseDir(const QString& userHome, const SimulatorCandidate& candidate);
[[nodiscard]] QString SimulatorUserCfgPath(const QString& userHome, const SimulatorCandidate& candidate);
[[nodiscard]] QString SimulatorFallbackCommunityDir(const QString& userHome, const SimulatorCandidate& candidate);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H
