#ifndef GSX_INTEGRATOR_CLIENT_SMARTSWITCHCUE_H
#define GSX_INTEGRATOR_CLIENT_SMARTSWITCHCUE_H

#include <string>

enum class SmartSwitchMove
{
    Flip,
    FlickEitherSide,
    TurnOn,
    Press
};

struct SmartSwitchCue
{
    std::string control{};
    std::string side{};
    SmartSwitchMove move{SmartSwitchMove::Flip};

    bool operator==(const SmartSwitchCue&) const = default;
};

#endif // GSX_INTEGRATOR_CLIENT_SMARTSWITCHCUE_H
