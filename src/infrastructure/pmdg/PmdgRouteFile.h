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
    static long long LatestWrite(const std::filesystem::path& directory,
                                 const std::string& origin,
                                 const std::string& destination);

private:
    PmdgRouteFile() = delete;
};

class PmdgRouteImport
{
public:
    void Observe(const std::optional<std::filesystem::path>& directory,
                 const std::string& origin,
                 const std::string& destination,
                 long long planEpoch);
    [[nodiscard]] bool Seen() const { return seen_; }

private:
    std::optional<long long> baseline_;
    long long planEpoch_ = 0;
    bool seen_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H
