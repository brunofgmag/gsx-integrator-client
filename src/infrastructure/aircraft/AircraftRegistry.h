#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTREGISTRY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTREGISTRY_H

#include <memory>
#include <string>
#include <vector>
#include "AircraftIdentity.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

enum class MatchField
{
    Title,
    AtcModel
};

enum class MatchOp
{
    Equals,
    StartsWith,
    Contains
};

struct MatchRule
{
    MatchField field;
    MatchOp op;
    const char* pattern;
};

using AircraftCreator = std::unique_ptr<Aircraft> (*)(VariableGateway* variableGateway,
                                                      AutomationStatus* status,
                                                      const AircraftIdentity& identity);

struct AircraftDescriptor
{
    const char* name;
    std::vector<MatchRule> rules;
    AircraftCreator create;
};

std::vector<const AircraftDescriptor*>& AircraftRegistry();

struct AircraftRegistration
{
    explicit AircraftRegistration(const AircraftDescriptor& descriptor);
};

[[nodiscard]] bool MatchText(const std::string& value, MatchOp op, const char* pattern);

[[nodiscard]] const AircraftDescriptor* MatchAircraft(const std::vector<const AircraftDescriptor*>& candidates,
                                                      const AircraftIdentity& identity);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTREGISTRY_H
