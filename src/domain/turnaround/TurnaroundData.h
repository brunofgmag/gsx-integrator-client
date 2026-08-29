#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H

struct CabinServiceProgress
{
    bool asked = false;
    bool requested = false;
    bool activeSeen = false;
};

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
    int boardingStallTicks = 0;
    int boardingCompletionAttempts = 0;
    bool deboardingBaselined = false;
    bool refuelingRequested = false;
    int fuelRequestStallTicks = 0;
    bool fuelRequestStalled = false;
    bool fuelPlanOverCapacity = false;
    bool servicesStalled = false;
    int servicesWaitSeconds = 0;
    int servicesOperatingTicks = 0;
    bool boardingRequested = false;
    bool deboardingRequested = false;
    bool pushbackRequested = false;
    bool jetwayOrStairsRequested = false;
    bool jetwayOrStairsCompleted = false;
    int stairsInPlaceTicks = 0;
    bool gpuRequested = false;
    bool chocksPlaced = false;
    bool chocksRemoved = false;
    bool doorsClosed = false;
    bool ownGroundEquipmentCleared = false;
    bool arrivalGpuRequested = false;
    bool arrivalChocksPlaced = false;
    bool arrivalDoorsClosed = false;
    bool cateringAsked = false;
    bool cateringRequested = false;
    bool gpuDismissRequested = false;
    int cateringWaitIntervals = 0;
    CabinServiceProgress lavatory;
    CabinServiceProgress water;
    CabinServiceProgress cleaning;
    int cabinWaitIntervals = 0;
    bool flightPlanRequested = false;
    bool repositionRequested = false;
    bool repositionCompleted = false;

    int stateTickCount = 0;

    void Reset() { *this = {}; }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H
