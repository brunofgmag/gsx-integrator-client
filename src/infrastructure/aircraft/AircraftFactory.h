#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H

#include <memory>
#include <vector>
#include "../../domain/ports/Aircraft.h"
#include "../../application/model/AircraftProfile.h"

class VariableGateway;
struct AutomationStatus;
struct AircraftDescriptor;

[[nodiscard]] std::unique_ptr<Aircraft> DetectAircraft(VariableGateway* variableGateway,
                                                       AutomationStatus* status,
                                                       const AircraftDescriptor** outDescriptor = nullptr);

[[nodiscard]] std::vector<AircraftProfileInfo> SupportedAircraftProfiles();

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
