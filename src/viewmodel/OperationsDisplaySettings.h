#ifndef GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H

#include <QtCore/QString>

class OperationsDisplaySettings
{
public:
    virtual ~OperationsDisplaySettings() = default;

    [[nodiscard]] virtual bool GetWeightIsLb() const = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_OPERATIONSDISPLAYSETTINGS_H
