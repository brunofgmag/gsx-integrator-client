#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H

#include <cstdint>

enum class TurnaroundHold : std::uint8_t
{
    None,
    AwaitingGsxSnapshot,
    JudgingSavedTurnaround,
    AwaitingResumeDecision,
    AwaitingGsxReadings,
    AwaitingAircraft,
};

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H
