#ifndef GSX_INTEGRATOR_CLIENT_PMDGOPTIONS_H
#define GSX_INTEGRATOR_CLIENT_PMDGOPTIONS_H

#include <filesystem>
#include <optional>
#include <string>

class PmdgOptions
{
public:
    static std::optional<std::filesystem::path> PathFor(const std::string& aircraftName);
    static bool HasDataBroadcast(const std::string& iniText);
    static std::string WithDataBroadcast(const std::string& iniText);
    static std::optional<bool> ReadDataBroadcast(const std::filesystem::path& iniPath);
    static bool EnableDataBroadcast(const std::filesystem::path& iniPath);

private:
    PmdgOptions() = delete;
};

#endif // GSX_INTEGRATOR_CLIENT_PMDGOPTIONS_H
