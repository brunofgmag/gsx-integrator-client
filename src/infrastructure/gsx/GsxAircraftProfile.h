#ifndef GSX_INTEGRATOR_CLIENT_GSXAIRCRAFTPROFILE_H
#define GSX_INTEGRATOR_CLIENT_GSXAIRCRAFTPROFILE_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class GsxAircraftProfile
{
public:
    static std::vector<std::filesystem::path> ProfileRootsFor(const std::string& aircraftName);
    static bool FlagsMissingProfile(const std::string& aircraftName);
    static std::vector<std::filesystem::path> FindCfgs(const std::vector<std::filesystem::path>& roots);
    static std::optional<int> ReadRefueling(const std::filesystem::path& cfgPath);
    static bool WriteRefueling(const std::filesystem::path& cfgPath, int value);

private:
    GsxAircraftProfile() = delete;
};

#endif // GSX_INTEGRATOR_CLIENT_GSXAIRCRAFTPROFILE_H
