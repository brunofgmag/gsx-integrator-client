#ifndef GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H
#define GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H

#include <cstdint>
#include <string>
#include "GsxRemoteState.h"

class QJsonObject;
class QJsonValue;

enum class GsxPatchOutcome : std::uint8_t
{
    Applied,
    Discarded,
    Unknown
};

class GsxRemoteStateReducer
{
public:
    GsxRemoteStateReducer() = delete;

    static void ApplySnapshot(GsxRemoteState& state, const QJsonObject& snapshot);
    static GsxPatchOutcome ApplyPatch(GsxRemoteState& state, const std::string& path,
                                      const QJsonValue& value);
    static void ApplyConnection(GsxRemoteState& state, bool connected);
};

#endif //GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H
