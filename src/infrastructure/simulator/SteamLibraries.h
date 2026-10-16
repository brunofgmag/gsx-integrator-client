#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STEAMLIBRARIES_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STEAMLIBRARIES_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

[[nodiscard]] QStringList ParseSteamLibraryPaths(const QByteArray& libraryFoldersVdf);
[[nodiscard]] QStringList DefaultSteamappsDirs(const QString& machineHome);
[[nodiscard]] QStringList SteamLibrarySteamappsDirs(const QStringList& steamappsDirs);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STEAMLIBRARIES_H
