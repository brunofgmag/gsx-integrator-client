#ifndef GSX_INTEGRATOR_CLIENT_FLIGHTPLANSOURCE_H
#define GSX_INTEGRATOR_CLIENT_FLIGHTPLANSOURCE_H

class FlightPlanSource
{
public:
    virtual ~FlightPlanSource() = default;
    virtual void RequestLatest() = 0;
};

#endif //GSX_INTEGRATOR_CLIENT_FLIGHTPLANSOURCE_H
