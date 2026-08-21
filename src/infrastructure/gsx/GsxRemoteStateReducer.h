#ifndef GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H
#define GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H

#include <string>
#include "GsxRemoteState.h"

class QJsonObject;
class QJsonValue;

enum class GsxPatchOutcome
{
    Applied,
    Discarded,
    Unknown
};

class GsxRemoteStateReducer
{
public:
    static void ApplySnapshot(GsxRemoteState& state, const QJsonObject& snapshot);
    static GsxPatchOutcome ApplyPatch(GsxRemoteState& state, const std::string& path,
                                      const QJsonValue& value);

private:
    GsxRemoteStateReducer() = delete;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXREMOTESTATEREDUCER_H
