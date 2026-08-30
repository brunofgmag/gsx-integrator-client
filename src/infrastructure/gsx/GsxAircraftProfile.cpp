#include "GsxAircraftProfile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "../aircraft/fenix/FenixA32x.h"
#include "../aircraft/pmdg/Pmdg737.h"
#include "../aircraft/pmdg/Pmdg777.h"
#include "../aircraft/toliss/TolissA340.h"
#include "../aircraft/tfdi/TfdiMd11.h"

namespace
{
    constexpr auto kAircraftSection = "aircraft";
    constexpr auto kRefuelingKey = "refueling";

    struct ProfileFolder
    {
        const char* aircraftName;
        const char* folder;
    };

    constexpr std::array kProfileFolders = {
        ProfileFolder{TolissA340::kName, "airbus-a346-pro"},
        ProfileFolder{TolissA340::kName, "aerosoft-a340-600-pro"},
        ProfileFolder{TfdiMd11::kName, "tfdi_design_md-11"},
        ProfileFolder{FenixA32x::kNameA319, "FNX_32X"},
        ProfileFolder{FenixA32x::kNameA320, "FNX_32X"},
        ProfileFolder{FenixA32x::kNameA321, "FNX_32X"},
        ProfileFolder{Pmdg777::kName200Er, "PMDG 777-200ER"},
        ProfileFolder{Pmdg777::kName200Lr, "PMDG 777-200LR"},
        ProfileFolder{Pmdg777::kName300Er, "PMDG 777-300ER"},
        ProfileFolder{Pmdg777::kNameFreighter, "PMDG 777F"},
        ProfileFolder{Pmdg737::kNamePax800, "PMDG 737-800"},
        ProfileFolder{Pmdg737::kNameBcf800, "PMDG 737-800"},
        ProfileFolder{Pmdg737::kNameBdsf800, "PMDG 737-800"},
        ProfileFolder{Pmdg737::kNameBbj2, "PMDG 737-800"}
    };

    std::string Trim(const std::string& text)
    {
        const auto begin = text.find_first_not_of(" \t");
        if (begin == std::string::npos)
        {
            return {};
        }

        const auto end = text.find_last_not_of(" \t");
        return text.substr(begin, end - begin + 1);
    }

    std::string ToLower(std::string text)
    {
        std::ranges::transform(text, text.begin(),
                               [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::optional<std::string> SectionName(const std::string& trimmedLine)
    {
        if (trimmedLine.size() < 2 || trimmedLine.front() != '[' || trimmedLine.back() != ']')
        {
            return std::nullopt;
        }

        return ToLower(Trim(trimmedLine.substr(1, trimmedLine.size() - 2)));
    }

    bool IsRefuelingLine(const std::string& line)
    {
        const auto equals = line.find('=');
        if (equals == std::string::npos)
        {
            return false;
        }

        return ToLower(Trim(line.substr(0, equals))) == kRefuelingKey;
    }

    struct LineView
    {
        std::string text;
        std::size_t start = 0;
        std::size_t end = 0;
        std::size_t next = 0;
    };

    LineView NextLine(const std::string& content, const std::size_t lineStart)
    {
        const std::size_t newline = content.find('\n', lineStart);
        const std::size_t next = newline == std::string::npos ? content.size() : newline + 1;
        std::size_t end = newline == std::string::npos ? content.size() : newline;
        if (end > lineStart && content[end - 1] == '\r')
        {
            --end;
        }

        return {content.substr(lineStart, end - lineStart), lineStart, end, next};
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

    bool WriteContent(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << content;

        return output.good();
    }
}

std::vector<std::filesystem::path> GsxAircraftProfile::ProfileRootsFor(const std::string& aircraftName)
{
    char* appData = nullptr;
    size_t appDataLength = 0;
    if (_dupenv_s(&appData, &appDataLength, "APPDATA") != 0 || appData == nullptr)
    {
        return {};
    }

    const std::filesystem::path airplanes =
        std::filesystem::path(appData) / "Virtuali" / "Airplanes";

    std::free(appData);

    std::vector<std::filesystem::path> roots;
    for (const auto& [name, folder] : kProfileFolders)
    {
        if (aircraftName == name)
        {
            roots.push_back(airplanes / folder);
        }
    }

    return roots;
}

bool GsxAircraftProfile::FlagsMissingProfile(const std::string& aircraftName)
{
    return aircraftName == TolissA340::kName;
}

std::vector<std::filesystem::path> GsxAircraftProfile::FindCfgs(const std::vector<std::filesystem::path>& roots)
{
    std::vector<std::filesystem::path> cfgs;
    for (const auto& root : roots)
    {
        std::error_code openError;
        std::filesystem::recursive_directory_iterator entries(
            root, std::filesystem::directory_options::skip_permission_denied, openError);
        if (openError)
        {
            continue;
        }

        for (const auto& entry : entries)
        {
            std::error_code entryError;
            if (entry.is_regular_file(entryError)
                && ToLower(entry.path().filename().string()) == "gsx.cfg")
            {
                cfgs.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(cfgs);
    return cfgs;
}

std::optional<int> GsxAircraftProfile::ReadRefueling(const std::filesystem::path& cfgPath)
{
    const std::optional<std::string> content = ReadContent(cfgPath);
    if (!content.has_value())
    {
        return std::nullopt;
    }

    std::string section;
    LineView line;
    for (std::size_t pos = 0; pos < content->size(); pos = line.next)
    {
        line = NextLine(*content, pos);

        const std::string trimmed = Trim(line.text);
        if (const auto name = SectionName(trimmed))
        {
            section = *name;
            continue;
        }

        if (section != kAircraftSection || !IsRefuelingLine(trimmed))
        {
            continue;
        }

        try
        {
            return std::stoi(Trim(trimmed.substr(trimmed.find('=') + 1)));
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

bool GsxAircraftProfile::WriteRefueling(const std::filesystem::path& cfgPath, const int value)
{
    std::optional<std::string> content = ReadContent(cfgPath);
    if (!content.has_value())
    {
        return false;
    }

    const std::string eol = content->find("\r\n") != std::string::npos ? "\r\n" : "\n";
    const std::string refuelingLine = std::string(kRefuelingKey) + " = " + std::to_string(value);

    std::string section;
    std::size_t afterAircraftHeader = std::string::npos;
    LineView line;
    for (std::size_t pos = 0; pos < content->size(); pos = line.next)
    {
        line = NextLine(*content, pos);

        const std::string trimmed = Trim(line.text);
        if (const auto name = SectionName(trimmed))
        {
            section = *name;
            if (section == kAircraftSection)
            {
                afterAircraftHeader = line.next;
            }
        }
        else if (section == kAircraftSection && IsRefuelingLine(trimmed))
        {
            content->replace(line.start, line.end - line.start, refuelingLine);

            return WriteContent(cfgPath, *content);
        }
    }

    if (afterAircraftHeader == std::string::npos)
    {
        return false;
    }

    content->insert(afterAircraftHeader, refuelingLine + eol);

    return WriteContent(cfgPath, *content);
}
