#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDMATH_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDMATH_H

#include <algorithm>
#include <cmath>

namespace turnaround
{
    inline constexpr double kTickSeconds = 1.0;
    inline constexpr double kWeightEpsilonKg = 50.0;

    [[nodiscard]] inline double ProgressPercent(
        const double initialValue,
        const double currentValue,
        const double targetValue)
    {
        if (!std::isfinite(initialValue) || !std::isfinite(currentValue) || !std::isfinite(targetValue))
        {
            return 0.0;
        }

        const double totalChange = targetValue - initialValue;
        if (std::abs(totalChange) <= kWeightEpsilonKg)
        {
            return std::abs(targetValue - currentValue) <= kWeightEpsilonKg ? 100.0 : 0.0;
        }

        return std::clamp(
            (currentValue - initialValue) / totalChange * 100.0,
            0.0,
            100.0);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDMATH_H
