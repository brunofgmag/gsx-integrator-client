#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBRELEASEPARSER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBRELEASEPARSER_H

#include <optional>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include "../../application/model/UpdateInfo.h"

[[nodiscard]] std::optional<UpdateInfo> ParseLatestRelease(const QByteArray& json);
[[nodiscard]] QString ParseSha256File(const QByteArray& content);
[[nodiscard]] bool IsNewerVersion(const QString& tagName, const QString& currentVersion);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GITHUBRELEASEPARSER_H
