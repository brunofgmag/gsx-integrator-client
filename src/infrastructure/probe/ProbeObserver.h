#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H

#include <string>
#include <unordered_map>
#include <QtCore/QString>

class Aircraft;
class SimConnectSession;
class VariableGateway;

class ProbeObserver
{
public:
    void Observe(const Aircraft& aircraft, VariableGateway& variables, const std::string& profileId);
    void MaybeFireEvent(SimConnectSession& session);

private:
    struct Track
    {
        double min = 0.0;
        double max = 0.0;
        bool seen = false;
    };

    const Track& Follow(VariableGateway& variables, const char* name);
    void ReportWatchList(VariableGateway& variables, const QString& id);
    void MaybeSetLVar(VariableGateway& variables);

    std::unordered_map<std::string, Track> tracks_;
    long long lastObservedMs_ = 0;
    bool setLVarSent_ = false;
    bool fireEventSent_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEOBSERVER_H
