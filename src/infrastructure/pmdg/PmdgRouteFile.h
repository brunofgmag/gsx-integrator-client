#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H

#include <filesystem>
#include <optional>
#include <string>

class PmdgRouteFile
{
public:
    PmdgRouteFile() = delete;

    static std::optional<std::filesystem::path> DirectoryFor(const std::string& aircraftName);
    static bool NamesPlan(const std::filesystem::path& file,
                          const std::string& origin,
                          const std::string& destination);
    static std::optional<long long> ParseGeneratedAt(const std::string& line);
    static std::optional<long long> GeneratedAt(const std::filesystem::path& file);
    static long long LatestWrite(const std::filesystem::path& directory);
    static std::optional<std::filesystem::path> NewestWrittenAfter(const std::filesystem::path& directory,
                                                                   long long baseline);
};

class PmdgRouteImport
{
public:
    void Observe(const std::optional<std::filesystem::path>& directory,
                 const std::string& origin,
                 const std::string& destination,
                 long long planEpoch);
    void Restart();
    [[nodiscard]] bool Seen() const { return seen_; }
    [[nodiscard]] bool NewerThanTheOfp() const { return newerThanTheOfp_; }

private:
    void Judge(const std::filesystem::path& route,
               const std::string& origin,
               const std::string& destination,
               long long planEpoch);

    std::optional<long long> baseline_;
    bool seen_ = false;
    bool newerThanTheOfp_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGROUTEFILE_H
