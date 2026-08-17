#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H

#include <filesystem>
#include <optional>
#include <string>

class PmdgRouteFile
{
public:
    static std::optional<std::filesystem::path> DirectoryFor(const std::string& aircraftName);
    static bool NamesPlan(const std::filesystem::path& file,
                          const std::string& origin,
                          const std::string& destination);
    static bool ImportedSince(const std::filesystem::path& directory,
                              const std::string& origin,
                              const std::string& destination,
                              long long epochSeconds);

private:
    PmdgRouteFile() = delete;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H
