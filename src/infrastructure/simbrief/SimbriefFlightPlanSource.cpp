#include "SimbriefFlightPlanSource.h"

#include "SimbriefClient.h"

SimbriefFlightPlanSource::SimbriefFlightPlanSource(SimbriefClient* client)
    : client_(client)
{
}

void SimbriefFlightPlanSource::RequestLatest()
{
    (void)client_->Reload();
}
