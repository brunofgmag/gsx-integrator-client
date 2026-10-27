#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSHOLDS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSHOLDS_H

#include <array>
#include "../../fslabs/FslabsGroundGateway.h"

class VariableGateway;

class FslabsHolds
{
public:
    FslabsHolds(VariableGateway& variables, FslabsGroundGateway& ground);

    void Advance();
    void ResetForNewTurnaround();

private:
    struct Follow
    {
        FslabsHold hold;
        const char* loaderStateLVar;
        const char* name;
        bool loaderWasServing = false;
        bool closeIsDue = false;
        int toggles = 0;
        int ticksSinceToggle = 0;
    };

    void Advance(Follow& follow);
    void CloseTheHold(Follow& follow);

    VariableGateway* variables_;
    FslabsGroundGateway* ground_;
    std::array<Follow, 2> follows_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSHOLDS_H
