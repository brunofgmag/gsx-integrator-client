#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H

#include <cstdint>
#include <optional>

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
}

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDKEYJUDGEMENT_H
