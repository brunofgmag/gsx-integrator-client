#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EXEXML_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EXEXML_H

#include <vector>
#include <QtCore/QString>

struct ExeXmlTarget
{
    QString label;
    QString path;
};

[[nodiscard]] bool ExeXmlAddUpdate(const QString& exeXmlPath, const QString& exePath, const QString& appName);
[[nodiscard]] bool ExeXmlRemove(const QString& exeXmlPath, const QString& exeName);
[[nodiscard]] bool ExeXmlHasEnabledEntry(const QString& exeXmlPath, const QString& exeName);
[[nodiscard]] std::vector<ExeXmlTarget> CandidateExeXmlTargets(const QString& homeDir);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EXEXML_H
