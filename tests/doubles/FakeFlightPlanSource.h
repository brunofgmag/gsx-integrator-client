#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEFLIGHTPLANSOURCE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEFLIGHTPLANSOURCE_H

#include "../../src/domain/model/AutomationStatus.h"
#include "../../src/domain/ports/FlightPlanSource.h"

class FakeFlightPlanSource final : public FlightPlanSource
{
public:
    explicit FakeFlightPlanSource(AutomationStatus* status)
        : status_(status)
    {
    }

    int latestRequests = 0;

    void RequestLatest() override
    {
        ++latestRequests;
        status_->flightPlanStatus = FlightPlanStatus::Fetching;
    }

private:
    AutomationStatus* status_;
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEFLIGHTPLANSOURCE_H
