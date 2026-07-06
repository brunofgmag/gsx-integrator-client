#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKESETTINGSREPOSITORY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKESETTINGSREPOSITORY_H

#include "../../src/application/ports/SettingsRepository.h"

class FakeSettingsRepository final : public SettingsRepository
{
public:
    AppSettings stored;
    bool saveResult = true;
    int saveCalls = 0;

    [[nodiscard]] AppSettings Load() const override
    {
        return stored;
    }

    [[nodiscard]] bool Save(const AppSettings& settings) override
    {
        ++saveCalls;
        if (saveResult)
        {
            stored = settings;
        }
        return saveResult;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKESETTINGSREPOSITORY_H
