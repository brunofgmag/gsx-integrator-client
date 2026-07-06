#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_SIMBRIEFOFPPARSER_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_SIMBRIEFOFPPARSER_H

#include <optional>
#include <string_view>
#include "../../domain/model/FlightPlan.h"

[[nodiscard]] std::optional<FlightPlan> ParseSimbriefOfp(std::string_view xml);

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_SIMBRIEFOFPPARSER_H
