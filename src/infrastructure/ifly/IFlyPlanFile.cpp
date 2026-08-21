#include "IFlyPlanFile.h"

#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>
#include "../logging/LogMacros.h"
#include "../simbrief/SimbriefOfpParser.h"

namespace
{
    constexpr auto kPackage = "ifly-aircraft-737max8";
    constexpr auto kPlanFile = "activeflightplan.xml";
    constexpr std::size_t kPrefixBytes = 4096;

    std::string ReadPrefix(const std::filesystem::path& file)
    {
        std::ifstream stream(file, std::ios::binary);
        if (!stream)
        {
            return {};
        }

        std::string prefix(kPrefixBytes, '\0');
        stream.read(prefix.data(), static_cast<std::streamsize>(kPrefixBytes));
        prefix.resize(static_cast<std::size_t>(stream.gcount()));

        return prefix;
    }
}

std::optional<std::filesystem::path> IFlyPlanFile::DirectoryUnder(const std::filesystem::path& appData)
{
    const std::filesystem::path directory = appData
        / "Microsoft Flight Simulator 2024" / "WASM" / "MSFS2020" / kPackage
        / "work" / "navdata" / "FLTPLAN";

    std::error_code error;
    if (!std::filesystem::is_directory(directory, error))
    {
        return std::nullopt;
    }

    return directory;
}

std::optional<std::filesystem::path> IFlyPlanFile::DirectoryFor()
{
    char* appData = nullptr;
    std::size_t appDataLength = 0;
    if (_dupenv_s(&appData, &appDataLength, "APPDATA") != 0 || appData == nullptr)
    {
        return std::nullopt;
    }

    const std::optional<std::filesystem::path> directory = DirectoryUnder(appData);

    std::free(appData);

    return directory;
}

long long IFlyPlanFile::PlanEpochIn(const std::filesystem::path& directory)
{
    return ParseSimbriefPlanEpoch(ReadPrefix(directory / kPlanFile));
}

void IFlyPlanImport::Observe(const std::optional<std::filesystem::path>& directory, const long long planEpoch)
{
    if (planEpoch != planEpoch_)
    {
        planEpoch_ = planEpoch;
        seen_ = false;
    }

    if (seen_)
    {
        return;
    }

    const long long fileEpoch = directory.has_value() ? IFlyPlanFile::PlanEpochIn(*directory) : 0;

    blind_ = fileEpoch == 0;
    if (blind_)
    {
        if (!warned_)
        {
            warned_ = true;
            LOG_INFO("iFly: no readable flight plan in the aircraft folder; the flow will not wait for one");
        }

        return;
    }

    seen_ = planEpoch != 0 && fileEpoch == planEpoch;
}
