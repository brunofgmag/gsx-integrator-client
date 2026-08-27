#ifndef GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H

#include <QtCore/QString>

class OperationsDisplaySettings
{
public:
    virtual ~OperationsDisplaySettings() = default;

    [[nodiscard]] virtual bool GetWeightIsLb() const = 0;
    [[nodiscard]] virtual bool GetAutoStartFlow() const = 0;
    [[nodiscard]] virtual bool GetAutoStartLoading() const = 0;
    [[nodiscard]] virtual QString GetFuelRateText() const = 0;
    [[nodiscard]] virtual QString GetFuelRateUnitText() const = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H
