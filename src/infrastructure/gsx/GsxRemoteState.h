#ifndef GSX_INTEGRATOR_CLIENT_GSXREMOTESTATE_H
#define GSX_INTEGRATOR_CLIENT_GSXREMOTESTATE_H

#include <string>
#include <vector>

struct GsxRemoteService
{
    std::string id;
    int stateRaw = 0;
    bool canTrigger = false;
};

struct GsxRemoteMenu
{
    std::string title;
    std::vector<std::string> entries;
    std::vector<bool> disabled;
    bool shown = false;
};

struct GsxRemoteState
{
    std::string simbriefStatus;
    std::string simbriefError;
    GsxRemoteMenu menu;
    std::vector<GsxRemoteService> services;
};

[[nodiscard]] inline const GsxRemoteService* FindService(const GsxRemoteState& s, const std::string& id)
{
    for (const auto& service : s.services)
    {
        if (service.id == id)
        {
            return &service;
        }
    }

    return nullptr;
}

#endif //GSX_INTEGRATOR_CLIENT_GSXREMOTESTATE_H
