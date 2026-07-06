#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H

#include <memory>
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

[[nodiscard]] std::unique_ptr<Aircraft> DetectAircraft(VariableGateway* variableGateway, AutomationStatus* status);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
