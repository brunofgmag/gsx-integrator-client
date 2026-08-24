#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H

#include "../../src/viewmodel/OperationsDisplaySettings.h"

class FakeOperationsDisplaySettings final : public OperationsDisplaySettings
{
public:
    bool weightIsLb = false;

    [[nodiscard]] bool GetWeightIsLb() const override { return weightIsLb; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H
