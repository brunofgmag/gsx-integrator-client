#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEDESKTOPENTRY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEDESKTOPENTRY_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

enum class DesktopEntryOutcome
{
    Written,
    UpToDate,
    LeftAlone,
    Failed,
};

[[nodiscard]] QString QuoteDesktopExec(const QString& path);
[[nodiscard]] QString BuildAppImageDesktopEntry(const QString& appImagePath, const QString& iconPath);
[[nodiscard]] QString AppImageDesktopEntryPath(const QString& dataHome);
[[nodiscard]] QString AppImageDesktopIconPath(const QString& dataHome);
DesktopEntryOutcome EnsureAppImageDesktopEntry(const QString& appImagePath,
                                               const QString& dataHome,
                                               const QByteArray& iconBytes);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEDESKTOPENTRY_H
