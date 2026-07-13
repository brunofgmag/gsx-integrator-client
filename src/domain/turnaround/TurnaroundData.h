#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H

struct TurnaroundData
{
    double plannedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    int plannedPassengers = 0;
    int boardedPassengers = 0;

    double loadedFuelKg = 0.0;
    double initialFuelKg = 0.0;
    double loadedZfwKg = 0.0;
    double initialZfwKg = 0.0;

    double fuelProgress = 0.0;
    double boardingProgress = 0.0;
    double deboardingProgress = 0.0;

    bool loadingConfirmed = false;
    bool loadingStartNotified = false;
    bool refuelBaselined = false;
    double refuelStallSampleKg = 0.0;
    int refuelStallTicks = 0;
    bool refuelCompletionForced = false;
    bool boardingBaselined = false;
    bool deboardingBaselined = false;
    bool refuelingRequested = false;
    bool boardingRequested = false;
    bool deboardingRequested = false;
    bool pushbackRequested = false;
    bool disconnectConfirmed = false;
    bool jetwayOrStairsRequested = false;
    bool jetwayOrStairsCompleted = false;
    int jetwayOrStairsAttempts = 0;
    bool gpuRequested = false;
    bool cateringRequested = false;
    bool gpuDismissRequested = false;
    int gpuDismissAttempts = 0;
    bool flightPlanRequested = false;
    bool repositionRequested = false;
    bool repositionCompleted = false;

    int stateTickCount = 0;

    void Reset() { *this = {}; }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H
