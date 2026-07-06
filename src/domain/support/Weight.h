#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H

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
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WEIGHT_H
