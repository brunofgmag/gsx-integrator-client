#include "GsxAircraftProfile.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "../aircraft/TolissA340.h"
#include "../aircraft/TfdiMd11.h"

namespace
{
    constexpr auto kAircraftSection = "aircraft";
    constexpr auto kRefuelingKey = "refueling";

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

    if (aircraftName == TolissA340::kName)
    {
        return {airplanes / "airbus-a346-pro", airplanes / "aerosoft-a340-600-pro"};
    }

    if (aircraftName == TfdiMd11::kName)
    {
        return {airplanes / "tfdi_design_md-11"};
    }

    return {};
}

bool GsxAircraftProfile::FlagsMissingProfile(const std::string& aircraftName)
{
    return aircraftName == TolissA340::kName;
}

std::vector<std::filesystem::path> GsxAircraftProfile::FindCfgs(
    const std::vector<std::filesystem::path>& roots)
{
    std::vector<std::filesystem::path> cfgs;
    for (const auto& root : roots)
    {
        std::error_code fileError;
        std::filesystem::recursive_directory_iterator entries(
            root, std::filesystem::directory_options::skip_permission_denied, fileError);
        if (fileError)
        {
            continue;
        }

        for (const auto& entry : entries)
        {
            if (entry.is_regular_file(fileError)
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
    std::ifstream file(cfgPath);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    std::string line;
    std::string section;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const std::string trimmed = Trim(line);
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
    std::ifstream input(cfgPath, std::ios::binary);
    if (!input.is_open())
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();
    input.close();

    const std::string eol = content.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    const std::string refuelingLine = std::string(kRefuelingKey) + " = " + std::to_string(value);

    std::string section;
    std::size_t aircraftSectionEnd = std::string::npos;
    std::size_t lineStart = 0;
    while (lineStart < content.size())
    {
        const std::size_t newline = content.find('\n', lineStart);
        const std::size_t nextLine = newline == std::string::npos ? content.size() : newline + 1;
        std::size_t lineEnd = newline == std::string::npos ? content.size() : newline;
        if (lineEnd > lineStart && content[lineEnd - 1] == '\r')
        {
            --lineEnd;
        }

        const std::string line = content.substr(lineStart, lineEnd - lineStart);
        const std::string trimmed = Trim(line);
        if (const auto name = SectionName(trimmed))
        {
            section = *name;
            if (section == kAircraftSection)
            {
                aircraftSectionEnd = nextLine;
            }
        }
        else if (section == kAircraftSection && IsRefuelingLine(trimmed))
        {
            content.replace(lineStart, lineEnd - lineStart, refuelingLine);
            std::ofstream output(cfgPath, std::ios::binary | std::ios::trunc);
            output << content;
            return output.good();
        }

        lineStart = nextLine;
    }

    if (aircraftSectionEnd == std::string::npos)
    {
        return false;
    }

    content.insert(aircraftSectionEnd, refuelingLine + eol);
    std::ofstream output(cfgPath, std::ios::binary | std::ios::trunc);
    output << content;

    return output.good();
}
