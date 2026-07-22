#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTREGISTRY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AIRCRAFTREGISTRY_H

#include <memory>
#include <string>
#include <vector>
#include "AircraftIdentity.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;
class CommBusBridgeGateway;

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

struct AircraftContext
{
    VariableGateway* variableGateway = nullptr;
    AutomationStatus* status = nullptr;
    CommBusBridgeGateway* commBusBridge = nullptr;
};

using AircraftCreator = std::unique_ptr<Aircraft> (*)(const AircraftContext& context, const AircraftIdentity& identity);

struct AircraftDescriptor
{
    const char* name;
    std::vector<MatchRule> rules;
    AircraftCreator create;
    const char* id = "";
    const char* shortCode = "";
    RefuelBy refuelBy = RefuelBy::Gsx;
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
