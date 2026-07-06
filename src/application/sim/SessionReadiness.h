#ifndef GSX_INTEGRATOR_CLIENT_SESSIONREADINESS_H
#define GSX_INTEGRATOR_CLIENT_SESSIONREADINESS_H

#include "SimVersion.h"

namespace SessionReadiness
{
    inline bool Evaluate(const SimVersion version,
                         const double cameraState,
                         const double isAircraft,
                         const double isAvatar)
    {
        constexpr double kGlobalMenuCamera = 11.0;

        const bool offGlobalMenu = cameraState < kGlobalMenuCamera;

        if (version == SimVersion::Msfs2024)
        {
            return offGlobalMenu && isAircraft == 1.0 && isAvatar != 1.0;
        }

        return offGlobalMenu;
    }
}

#endif //GSX_INTEGRATOR_CLIENT_SESSIONREADINESS_H
