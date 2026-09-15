#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H

#include <memory>
#include <vector>
#include "AircraftRegistry.h"
#include "../../domain/ports/Aircraft.h"
#include "../../application/model/AircraftProfile.h"

[[nodiscard]] std::unique_ptr<Aircraft> DetectAircraft(const AircraftContext& context,
                                                       const AircraftDescriptor** outDescriptor = nullptr);

[[nodiscard]] std::vector<AircraftProfileInfo> SupportedAircraftProfiles();

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTFACTORY_H
