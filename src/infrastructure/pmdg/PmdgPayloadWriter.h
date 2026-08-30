#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGPAYLOADWRITER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGPAYLOADWRITER_H

#include <optional>

class PmdgTabletGateway;
class VariableReader;
struct AutomationStatus;

class PmdgPayloadWriter
{
public:
    PmdgPayloadWriter(PmdgTabletGateway& tablet, VariableReader& variables,
                      const AutomationStatus* status, bool cargoVariant);

    void Reset();
    void SetFuelKg(double fuelKg);
    void SetZfwKg(double zfwKg);
    void Trim();

private:
    PmdgTabletGateway& tablet_;
    VariableReader& variables_;
    const AutomationStatus* status_;
    bool cargoVariant_;
    int lastSentFuelLbs_ = -1;
    int lastSentPax_ = -1;
    int lastSentCargoLbs_ = -1;
    int lastProgressiveCargoLbs_ = -1;
    std::optional<double> rampStartZfwKg_;
    double lastRequestedZfwKg_ = 0.0;
    bool progressiveRampMoving_ = false;
    int zfwSettledTicks_ = 0;
    int zfwTrims_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGPAYLOADWRITER_H
