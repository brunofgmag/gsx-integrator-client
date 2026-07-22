#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SMARTSWITCH_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SMARTSWITCH_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

class VariableGateway;

class SmartSwitch
{
public:
    using Predicate = std::function<bool(double min, double max)>;

    SmartSwitch(VariableGateway& gateway, std::vector<std::string> lvars, Predicate pressed,
                std::optional<double> resetTo = std::nullopt);

    void Subscribe();
    [[nodiscard]] bool Consume();

private:
    VariableGateway& gateway_;
    std::vector<std::string> lvars_;
    Predicate pressed_;
    std::optional<double> resetTo_;
    bool subscribed_ = false;
    bool pending_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SMARTSWITCH_H
