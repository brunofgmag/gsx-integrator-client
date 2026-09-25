#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H

#include "../../src/viewmodel/OperationsDisplaySettings.h"

class FakeOperationsDisplaySettings final : public OperationsDisplaySettings
{
public:
    bool weightIsLb = false;
    bool autoStartFlow = false;
    bool autoStartLoading = false;
    QString fuelRateUnitText = QStringLiteral("kg/s");

    [[nodiscard]] bool GetWeightIsLb() const override { return weightIsLb; }
    [[nodiscard]] bool GetAutoStartFlow() const override { return autoStartFlow; }
    [[nodiscard]] bool GetAutoStartLoading() const override { return autoStartLoading; }
    [[nodiscard]] QString GetFuelRateUnitText() const override { return fuelRateUnitText; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEOPERATIONSDISPLAYSETTINGS_H
