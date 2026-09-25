#ifndef GSX_INTEGRATOR_CLIENT_SIMBRIEFFLIGHTPLANSOURCE_H
#define GSX_INTEGRATOR_CLIENT_SIMBRIEFFLIGHTPLANSOURCE_H

#include "../../domain/ports/FlightPlanSource.h"

class SimbriefClient;

class SimbriefFlightPlanSource final : public FlightPlanSource
{
public:
    explicit SimbriefFlightPlanSource(SimbriefClient* client);

    void RequestLatest() override;

private:
    SimbriefClient* client_;
};

#endif //GSX_INTEGRATOR_CLIENT_SIMBRIEFFLIGHTPLANSOURCE_H
