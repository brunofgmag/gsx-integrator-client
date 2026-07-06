#ifndef GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDSTATEFIXTURE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDSTATEFIXTURE_H

#include "tests/doubles/FakeAircraft.h"
#include "tests/doubles/FakeDomainLogger.h"
#include "tests/doubles/FakeGsxMenuGateway.h"
#include "tests/doubles/FakeGsxService.h"
#include "src/domain/model/AutomationStatus.h"
#include "src/domain/model/AutomationSettings.h"
#include "src/domain/turnaround/TurnaroundContext.h"

struct TurnaroundStateFixture
{
    AutomationStatus status;
    AutomationSettings settings;
    FakeGsxService gsxService;
    FakeGsxMenuGateway menuGateway;
    FakeDomainLogger logger;
    FakeAircraft aircraft;
    TurnaroundContext ctx;

    TurnaroundStateFixture()
    {
        ctx.status = &status;
        ctx.settings = &settings;
        ctx.gsxGateway = &gsxService;
        ctx.menuGateway = &menuGateway;
        ctx.aircraft = &aircraft;
        ctx.logger = &logger;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDSTATEFIXTURE_H
