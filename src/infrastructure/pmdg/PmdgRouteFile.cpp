#include "PmdgRouteFile.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <system_error>
#include "PmdgOptions.h"

namespace
{
    constexpr auto kFlightplansDir = "Flightplans";
    constexpr auto kRouteExtension = ".rte";

    std::string Lowered(const std::string& text)
    {
        std::string lowered = text;
        std::ranges::transform(lowered, lowered.begin(),
                               [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return lowered;
    }

    long long EpochSecondsOf(const std::filesystem::directory_entry& entry)
    {
        std::error_code error;
        const std::filesystem::file_time_type written = entry.last_write_time(error);
        if (error)
        {
            return 0;
        }

        const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(written);

        return std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count();
    }
}

std::optional<std::filesystem::path> PmdgRouteFile::DirectoryFor(const std::string& aircraftName)
{
    const std::optional<std::filesystem::path> optionsPath = PmdgOptions::PathFor(aircraftName);
    if (!optionsPath.has_value())
    {
        return std::nullopt;
    }

    return optionsPath->parent_path() / kFlightplansDir;
}

bool PmdgRouteFile::NamesPlan(const std::filesystem::path& file,
                              const std::string& origin,
                              const std::string& destination)
{
    if (origin.empty() || destination.empty())
    {
        return false;
    }

    if (Lowered(file.extension().string()) != kRouteExtension)
    {
        return false;
    }

    return Lowered(file.stem().string()).starts_with(Lowered(origin + destination));
}

long long PmdgRouteFile::LatestWrite(const std::filesystem::path& directory,
                                     const std::string& origin,
                                     const std::string& destination)
{
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error))
    {
        return 0;
    }

    long long latest = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error))
    {
        if (!entry.is_regular_file() || !NamesPlan(entry.path(), origin, destination))
        {
            continue;
        }

        latest = std::max(latest, EpochSecondsOf(entry));
    }

    return latest;
}

void PmdgRouteImport::Observe(const std::optional<std::filesystem::path>& directory,
                              const std::string& origin,
                              const std::string& destination,
                              const long long planEpoch)
{
    if (planEpoch != planEpoch_)
    {
        planEpoch_ = planEpoch;
        baseline_.reset();
        seen_ = false;
    }

    if (seen_ || !directory.has_value())
    {
        return;
    }

    const long long latest = PmdgRouteFile::LatestWrite(*directory, origin, destination);
    if (!baseline_.has_value())
    {
        baseline_ = latest;

        return;
    }

    seen_ = latest > *baseline_;
}
