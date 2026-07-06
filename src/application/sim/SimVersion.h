#ifndef GSX_INTEGRATOR_CLIENT_SIMVERSION_H
#define GSX_INTEGRATOR_CLIENT_SIMVERSION_H

#include <cctype>
#include <string_view>
#include <cstddef>

enum class SimVersion
{
    Unknown,
    Msfs2020,
    Msfs2024,
};

namespace SimVersionDetect
{
    inline constexpr std::string_view kAppName2020 = "KittyHawk";
    inline constexpr std::string_view kAppName2024 = "SunRise";

    inline bool EqualsIgnoreCase(const std::string_view& a, const std::string_view& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < a.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            {
                return false;
            }
        }

        return true;
    }

    inline SimVersion FromAppName(const std::string_view appName)
    {
        if (EqualsIgnoreCase(appName, kAppName2020))
        {
            return SimVersion::Msfs2020;
        }

        if (EqualsIgnoreCase(appName, kAppName2024))
        {
            return SimVersion::Msfs2024;
        }

        return SimVersion::Unknown;
    }
}

#endif //GSX_INTEGRATOR_CLIENT_SIMVERSION_H
