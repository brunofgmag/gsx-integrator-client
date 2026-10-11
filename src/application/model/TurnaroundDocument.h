#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDDOCUMENT_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDDOCUMENT_H

#include <map>
#include <optional>
#include <string>

#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/MemoryBag.h"
#include "../../domain/turnaround/TurnaroundCheckpoint.h"

struct TurnaroundKey
{
    std::string couatlId;
    std::string aircraftId;
    std::string aircraftTitle;
    std::string airportIcao;
    std::string parkingName;

    bool operator==(const TurnaroundKey&) const = default;
};

struct TurnaroundDocument
{
    TurnaroundKey key;
    std::optional<TurnaroundCheckpoint> checkpoint;
    std::optional<FlightPlan> plan;
    std::map<std::string, MemoryBag> memoryByOwner;
    bool repositioned = false;

    bool operator==(const TurnaroundDocument&) const = default;
};

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDDOCUMENT_H
