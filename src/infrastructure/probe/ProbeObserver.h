#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H

#include <string>
#include <unordered_map>

class Aircraft;
class VariableGateway;

class ProbeObserver
{
public:
    void Observe(const Aircraft& aircraft, VariableGateway& variables, const std::string& profileId);

private:
    struct Track
    {
        double min = 0.0;
        double max = 0.0;
        bool seen = false;
    };

    const Track& Follow(VariableGateway& variables, const char* name);

    std::unordered_map<std::string, Track> tracks_;
    long long lastObservedMs_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H
