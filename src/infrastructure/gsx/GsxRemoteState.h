#ifndef GSX_INTEGRATOR_CLIENT_GSXREMOTESTATE_H
#define GSX_INTEGRATOR_CLIENT_GSXREMOTESTATE_H

#include <string>
#include <vector>

struct GsxRemoteService
{
    std::string id;
    std::string displayName;
    std::string state;
    int stateRaw = 0;
    std::string stateText;
    std::string statusText;
    std::string progressText;
    bool canTrigger = false;
    bool canBypass = false;
};

struct GsxRemoteMenu
{
    std::string title;
    std::string subtitle;
    std::string header;
    std::vector<std::string> entries;
    std::vector<bool> disabled;
    bool shown = false;
};

struct GsxRemoteMessage
{
    std::string text;
    bool visible = false;
};

struct GsxRemoteState
{
    int sessionState = 0;
    std::string sessionStateText;
    GsxRemoteMessage message;
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
