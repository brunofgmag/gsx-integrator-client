#include "SimulatorCandidates.h"

#include "CaseInsensitivePath.h"

namespace
{
    constexpr auto kUserCfgName = "UserCfg.opt";
    constexpr auto kFallbackCommunitySubPath = "Packages/Community";
}

QString SimulatorBaseDir(const QString& userHome, const SimulatorCandidate& candidate)
{
    return ResolveCaseInsensitive(userHome, QLatin1String(candidate.baseSubPath));
}

QString SimulatorUserCfgPath(const QString& userHome, const SimulatorCandidate& candidate)
{
    return ResolveCaseInsensitive(SimulatorBaseDir(userHome, candidate), QLatin1String(kUserCfgName));
}

QString SimulatorFallbackCommunityDir(const QString& userHome, const SimulatorCandidate& candidate)
{
    return ResolveCaseInsensitive(SimulatorBaseDir(userHome, candidate), QLatin1String(kFallbackCommunitySubPath));
}
