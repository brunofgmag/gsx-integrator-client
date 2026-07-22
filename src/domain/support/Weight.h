#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H

#include <optional>

enum class WeightUnit
{
    Kg = 0,
    Lb = 1,
};

namespace weight
{
    constexpr double kLbToKg = 0.45359237;

    constexpr double LbToKg(const double pounds)
    {
        return pounds * kLbToKg;
    }

    constexpr double KgToLb(const double kilograms)
    {
        return kilograms / kLbToKg;
    }

    constexpr WeightUnit ResolveAutoWeightUnit(const bool simbriefConfigured,
                                               const bool simbriefReady,
                                               const WeightUnit simbriefUnit,
                                               const std::optional<WeightUnit> aircraftUnit)
    {
        if (simbriefConfigured && simbriefReady)
        {
            return simbriefUnit;
        }

        return aircraftUnit.value_or(WeightUnit::Kg);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H
