#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H

enum class TurnaroundHold
{
    None,
    AwaitingGsxSnapshot,
    JudgingSavedTurnaround,
    AwaitingResumeDecision,
    AwaitingGsxReadings,
    AwaitingAircraft,
};

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDHOLD_H
