#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H

struct SimulatorCandidate
{
    const char* label;
    const char* userCfgSubPath;
    const char* processName;
};

inline constexpr SimulatorCandidate kSimulatorCandidates[] = {
    {
        "MSFS 2020 (Steam)",
        "AppData/Roaming/Microsoft Flight Simulator/UserCfg.opt",
        "FlightSimulator.exe"
    },
    {
        "MSFS 2020 (Microsoft Store)",
        "AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/UserCfg.opt",
        "FlightSimulator.exe"
    },
    {
        "MSFS 2024 (Steam)",
        "AppData/Roaming/Microsoft Flight Simulator 2024/UserCfg.opt",
        "FlightSimulator2024.exe"
    },
    {
        "MSFS 2024 (Microsoft Store)",
        "AppData/Local/Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache/UserCfg.opt",
        "FlightSimulator2024.exe"
    },
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORCANDIDATES_H
