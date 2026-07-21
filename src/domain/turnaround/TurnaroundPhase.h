#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDPHASE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDPHASE_H

enum class TurnaroundPhase : int
{
    WaitingSupportedAircraft,
    WaitingAircraftReady,
    RepositionAircraft,
    PlaceGroundEquipment,
    CallServices,
    WaitingFlightPlan,
    WaitingPowerOn,
    CallCatering,
    RequestFuel,
    Refueling,
    RequestBoarding,
    Boarding,
    WaitingReadyToPush,
    WaitCatering,
    RemoveGroundEquipment,
    RequestPushback,
    WaitingPushbackToStart,
    WaitingForEngines,
    WaitingDeparture,
    OnFlight,
    WaitingEngineShutdown,
    PlaceArrivalGroundEquipment,
    RequestDeboarding,
    Deboarding,
    CabinServices,
    WaitingNewFlight,
    Count,
};

inline const char* TurnaroundPhaseToString(const TurnaroundPhase phase)
{
    switch (phase)
    {
    case TurnaroundPhase::WaitingSupportedAircraft: return "WaitingSupportedAircraft";
    case TurnaroundPhase::WaitingAircraftReady: return "WaitingAircraftReady";
    case TurnaroundPhase::WaitingFlightPlan: return "WaitingFlightPlan";
    case TurnaroundPhase::PlaceGroundEquipment: return "PlaceGroundEquipment";
    case TurnaroundPhase::CallServices: return "CallServices";
    case TurnaroundPhase::WaitingPowerOn: return "WaitingPowerOn";
    case TurnaroundPhase::CallCatering: return "CallCatering";
    case TurnaroundPhase::RequestFuel: return "RequestFuel";
    case TurnaroundPhase::Refueling: return "Refueling";
    case TurnaroundPhase::RequestBoarding: return "RequestBoarding";
    case TurnaroundPhase::Boarding: return "Boarding";
    case TurnaroundPhase::WaitingReadyToPush: return "WaitingReadyToPush";
    case TurnaroundPhase::WaitCatering: return "WaitCatering";
    case TurnaroundPhase::RemoveGroundEquipment: return "RemoveGroundEquipment";
    case TurnaroundPhase::RequestPushback: return "RequestPushback";
    case TurnaroundPhase::WaitingForEngines: return "WaitingForEngines";
    case TurnaroundPhase::WaitingPushbackToStart: return "WaitingPushbackToStart";
    case TurnaroundPhase::RepositionAircraft: return "RepositionAircraft";
    case TurnaroundPhase::WaitingDeparture: return "WaitingDeparture";
    case TurnaroundPhase::OnFlight: return "OnFlight";
    case TurnaroundPhase::PlaceArrivalGroundEquipment: return "PlaceArrivalGroundEquipment";
    case TurnaroundPhase::WaitingEngineShutdown: return "WaitingEngineShutdown";
    case TurnaroundPhase::RequestDeboarding: return "RequestDeboarding";
    case TurnaroundPhase::Deboarding: return "Deboarding";
    case TurnaroundPhase::CabinServices: return "CabinServices";
    case TurnaroundPhase::WaitingNewFlight: return "WaitingNewFlight";
    case TurnaroundPhase::Count:
    default: return "Unknown";
    };
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDPHASE_H
