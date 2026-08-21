#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLYPLANFILE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLYPLANFILE_H

#include <filesystem>
#include <optional>

class IFlyPlanFile
{
public:
    static std::optional<std::filesystem::path> DirectoryFor();
    static std::optional<std::filesystem::path> DirectoryUnder(const std::filesystem::path& appData);
    static long long PlanEpochIn(const std::filesystem::path& directory);

private:
    IFlyPlanFile() = delete;
};

class IFlyPlanImport
{
public:
    void Observe(const std::optional<std::filesystem::path>& directory, long long planEpoch);

    [[nodiscard]] bool Seen() const { return seen_; }
    [[nodiscard]] bool Blind() const { return blind_; }

private:
    long long planEpoch_ = 0;
    bool seen_ = false;
    bool blind_ = true;
    bool warned_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLYPLANFILE_H
