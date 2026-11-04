#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H

#include <cstdint>
#include <optional>
#include <string>

#include "model/TurnaroundDocument.h"
#include "../domain/turnaround/TurnaroundPhase.h"

enum class KeyVerdict : std::uint8_t
{
    Same,
    OnlyCouatlDiffers,
    Different,
    NotYetJudgeable,
};

namespace TurnaroundKeyJudgement
{
    [[nodiscard]] KeyVerdict Judge(const TurnaroundKey& saved,
                                   const TurnaroundKey& live,
                                   std::optional<TurnaroundPhase> savedPhase);

    [[nodiscard]] bool IsSameStand(const std::string& saved, const std::string& live);
}

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H
