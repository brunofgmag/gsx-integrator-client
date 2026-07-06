#ifndef GSX_INTEGRATOR_CLIENT_SETTINGSREPOSITORY_H
#define GSX_INTEGRATOR_CLIENT_SETTINGSREPOSITORY_H

#include "../model/AppSettings.h"

class SettingsRepository
{
public:
    virtual ~SettingsRepository() = default;

    [[nodiscard]] virtual AppSettings Load() const = 0;
    [[nodiscard]] virtual bool Save(const AppSettings& settings) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_SETTINGSREPOSITORY_H
