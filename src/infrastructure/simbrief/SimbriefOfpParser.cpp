#include "SimbriefOfpParser.h"

#include <cstdlib>
#include <string>
#include "../../domain/support/Weight.h"

namespace
{
    std::optional<std::string> ExtractTag(const std::string_view body, const std::string_view tag)
    {
        const std::string open = "<" + std::string(tag) + ">";
        const std::string close = "</" + std::string(tag) + ">";

        const std::size_t start = body.find(open);
        if (start == std::string_view::npos)
        {
            return std::nullopt;
        }

        const std::size_t valueStart = start + open.size();
        const std::size_t end = body.find(close, valueStart);
        if (end == std::string_view::npos)
        {
            return std::nullopt;
        }

        return std::string(body.substr(valueStart, end - valueStart));
    }

    std::optional<double> ParseDouble(const std::string& text)
    {
        char* parseEnd = nullptr;
        const double value = std::strtod(text.c_str(), &parseEnd);
        if (parseEnd == text.c_str())
        {
            return std::nullopt;
        }

        return value;
    }

    std::optional<int> ParseInt(const std::string& text)
    {
        char* parseEnd = nullptr;
        const long value = std::strtol(text.c_str(), &parseEnd, 10);
        if (parseEnd == text.c_str())
        {
            return std::nullopt;
        }

        return static_cast<int>(value);
    }
}

std::optional<FlightPlan> ParseSimbriefOfp(const std::string_view xml)
{
    if (xml.empty())
    {
        return std::nullopt;
    }

    const auto fuelValue = ExtractTag(xml, "plan_ramp");
    const auto zfwValue = ExtractTag(xml, "est_zfw");
    if (!fuelValue || !zfwValue)
    {
        return std::nullopt;
    }

    const auto parsedFuel = ParseDouble(*fuelValue);
    const auto parsedZfw = ParseDouble(*zfwValue);
    if (!parsedFuel || !parsedZfw)
    {
        return std::nullopt;
    }

    double fuelKg = *parsedFuel;
    double zfwKg = *parsedZfw;

    int passengers = 0;
    if (const auto passengerValue = ExtractTag(xml, "pax_count"); passengerValue)
    {
        const auto parsedPax = ParseInt(*passengerValue);
        if (!parsedPax)
        {
            return std::nullopt;
        }
        passengers = *parsedPax;
    }

    auto unit = WeightUnit::Kg;
    if (const auto units = ExtractTag(xml, "units"); units && *units == "lbs")
    {
        unit = WeightUnit::Lb;
        fuelKg = weight::LbToKg(fuelKg);
        zfwKg = weight::LbToKg(zfwKg);
    }

    if (fuelKg <= 0.0 || zfwKg <= 0.0 || passengers < 0)
    {
        return std::nullopt;
    }

    return FlightPlan{fuelKg, zfwKg, passengers, unit};
}
