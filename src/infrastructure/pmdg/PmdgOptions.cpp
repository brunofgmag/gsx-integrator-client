#include "PmdgOptions.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include "../aircraft/pmdg/Pmdg737.h"
#include "../aircraft/pmdg/Pmdg777.h"

namespace
{
    constexpr auto kSection = "[SDK]";
    constexpr auto kKey = "EnableDataBroadcast";
    constexpr auto kEnabled = "EnableDataBroadcast=1";

    struct PackageFile
    {
        const char* package;
        const char* iniName;
    };

    std::optional<PackageFile> PackageFor(const std::string& aircraftName)
    {
        if (aircraftName == Pmdg777::kName300Er)
        {
            return PackageFile{"pmdg-aircraft-77w", "777_Options.ini"};
        }

        if (aircraftName == Pmdg777::kNameFreighter)
        {
            return PackageFile{"pmdg-aircraft-77f", "777_Options.ini"};
        }

        if (aircraftName == Pmdg777::kName200Lr)
        {
            return PackageFile{"pmdg-aircraft-77l", "777_Options.ini"};
        }

        if (aircraftName == Pmdg777::kName200Er)
        {
            return PackageFile{"pmdg-aircraft-77er", "777_Options.ini"};
        }

        if (aircraftName == Pmdg737::kNamePax800 || aircraftName == Pmdg737::kNameBcf800
            || aircraftName == Pmdg737::kNameBdsf800 || aircraftName == Pmdg737::kNameBbj2)
        {
            return PackageFile{"pmdg-aircraft-738", "737_Options.ini"};
        }

        return std::nullopt;
    }

    std::string NewlineOf(const std::string& text)
    {
        return text.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    }

    std::size_t LineEnd(const std::string& text, const std::size_t start)
    {
        const std::size_t newline = text.find('\n', start);

        return newline == std::string::npos ? text.size() : newline + 1;
    }

    std::string Trimmed(const std::string& line)
    {
        const std::size_t begin = line.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
        {
            return {};
        }

        const std::size_t end = line.find_last_not_of(" \t\r\n");

        return line.substr(begin, end - begin + 1);
    }

    std::optional<std::string> ReadContent(const std::filesystem::path& path)
    {
        const std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
        {
            return std::nullopt;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();

        return buffer.str();
    }
}

std::optional<std::filesystem::path> PmdgOptions::PathFor(const std::string& aircraftName)
{
    const std::optional<PackageFile> package = PackageFor(aircraftName);
    if (!package.has_value())
    {
        return std::nullopt;
    }

    char* appData = nullptr;
    std::size_t appDataLength = 0;
    if (_dupenv_s(&appData, &appDataLength, "APPDATA") != 0 || appData == nullptr)
    {
        return std::nullopt;
    }

    const std::filesystem::path path = std::filesystem::path(appData)
        / "Microsoft Flight Simulator 2024" / "WASM" / "MSFS2024" / package->package
        / "work" / package->iniName;

    std::free(appData);

    return path;
}

bool PmdgOptions::HasDataBroadcast(const std::string& iniText)
{
    std::istringstream stream(iniText);
    std::string line;
    bool inSdkSection = false;
    while (std::getline(stream, line))
    {
        const auto begin = line.find_first_not_of(" \t\r");
        if (begin == std::string::npos)
        {
            continue;
        }

        if (line[begin] == '[')
        {
            inSdkSection = line.find(kSection, begin) == begin;
            continue;
        }

        if (inSdkSection && line.find(kKey) != std::string::npos
            && line.find('1', line.find('=')) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

std::string PmdgOptions::WithDataBroadcast(const std::string& iniText)
{
    if (HasDataBroadcast(iniText))
    {
        return iniText;
    }

    const std::string newline = NewlineOf(iniText);

    bool inSdkSection = false;
    std::size_t sectionBodyStart = std::string::npos;
    for (std::size_t start = 0; start < iniText.size();)
    {
        const std::size_t next = LineEnd(iniText, start);
        const std::string line = Trimmed(iniText.substr(start, next - start));

        if (!line.empty() && line.front() == '[')
        {
            inSdkSection = line.starts_with(kSection);
            if (inSdkSection)
            {
                sectionBodyStart = next;
            }
        }
        else if (inSdkSection && line.starts_with(kKey))
        {
            return iniText.substr(0, start) + kEnabled + newline + iniText.substr(next);
        }

        start = next;
    }

    if (sectionBodyStart != std::string::npos)
    {
        return iniText.substr(0, sectionBodyStart) + kEnabled + newline + iniText.substr(sectionBodyStart);
    }

    std::string prefix = iniText;
    if (!prefix.empty() && !prefix.ends_with("\n"))
    {
        prefix += newline;
    }

    return prefix + kSection + newline + kEnabled + newline;
}

std::optional<bool> PmdgOptions::ReadDataBroadcast(const std::filesystem::path& iniPath)
{
    const std::optional<std::string> content = ReadContent(iniPath);
    if (!content.has_value())
    {
        return std::nullopt;
    }

    return HasDataBroadcast(*content);
}

bool PmdgOptions::EnableDataBroadcast(const std::filesystem::path& iniPath)
{
    const std::optional<std::string> content = ReadContent(iniPath);
    if (!content.has_value())
    {
        return false;
    }

    std::ofstream output(iniPath, std::ios::binary | std::ios::trunc);
    output << WithDataBroadcast(*content);

    return output.good();
}
