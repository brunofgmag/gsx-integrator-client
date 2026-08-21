#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWATCHLIST_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWATCHLIST_H

#include <string>
#include <vector>

namespace probe
{
    enum class WatchKind
    {
        LVar,
        AVar,
        Dataref
    };

    struct WatchedVariable
    {
        WatchKind kind = WatchKind::LVar;
        std::string name;
        std::string unit;
    };

    [[nodiscard]] std::vector<WatchedVariable> ParseWatchList(const std::string& text);
    [[nodiscard]] const std::vector<WatchedVariable>& WatchList();
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWATCHLIST_H
