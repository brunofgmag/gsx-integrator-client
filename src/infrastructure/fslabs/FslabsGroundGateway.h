#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDGATEWAY_H

#include <cstdint>
#include <optional>

enum class FslabsHold : std::uint8_t { Forward, Aft };

enum class FslabsHoldState : std::uint8_t { Closed = 0, Opening = 1, Open = 2, Closing = 3 };

class FslabsGroundGateway
{
public:
    virtual ~FslabsGroundGateway() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual std::optional<FslabsHoldState> HoldState(FslabsHold hold) const = 0;
    virtual void ToggleHold(FslabsHold hold) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDGATEWAY_H
