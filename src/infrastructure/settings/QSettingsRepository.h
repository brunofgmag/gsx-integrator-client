#ifndef GSX_INTEGRATOR_CLIENT_QSETTINGSREPOSITORY_H
#define GSX_INTEGRATOR_CLIENT_QSETTINGSREPOSITORY_H

#include <QtCore/QSettings>
#include "../../application/ports/SettingsRepository.h"

class QSettingsRepository final : public SettingsRepository
{
public:
    QSettingsRepository() = default;

    [[nodiscard]] AppSettings Load() const override;
    [[nodiscard]] bool Save(const AppSettings& values) override;
};

#endif // GSX_INTEGRATOR_CLIENT_QSETTINGSREPOSITORY_H
