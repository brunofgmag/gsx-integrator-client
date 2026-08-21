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

    std::optional<std::string> ExtractNestedTag(const std::string_view body,
                                                const std::string_view outer,
                                                const std::string_view inner)
    {
        const std::string open = "<" + std::string(outer) + ">";
        const std::string close = "</" + std::string(outer) + ">";

        const std::size_t start = body.find(open);
        if (start == std::string_view::npos)
        {
            return std::nullopt;
        }

        const std::size_t end = body.find(close, start);
        if (end == std::string_view::npos)
        {
            return std::nullopt;
        }

        return ExtractTag(body.substr(start, end - start), inner);
    }

    long long ParseEpoch(const std::optional<std::string>& text)
    {
        if (!text)
        {
            return 0;
        }

        char* parseEnd = nullptr;
        const long long value = std::strtoll(text->c_str(), &parseEnd, 10);

        return parseEnd == text->c_str() ? 0 : value;
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

long long ParseSimbriefPlanEpoch(const std::string_view xml)
{
    return ParseEpoch(ExtractNestedTag(xml, "params", "time_generated"));
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

    const auto units = ExtractTag(xml, "units");
    const bool ofpInPounds = units && *units == "lbs";
    const auto unit = ofpInPounds ? WeightUnit::Lb : WeightUnit::Kg;
    const double fuelKg = ofpInPounds ? weight::LbToKg(*parsedFuel) : *parsedFuel;
    const double zfwKg = ofpInPounds ? weight::LbToKg(*parsedZfw) : *parsedZfw;

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

    if (fuelKg <= 0.0 || zfwKg <= 0.0 || passengers < 0)
    {
        return std::nullopt;
    }

    FlightPlan plan{fuelKg, zfwKg, passengers, unit};
    plan.origin = ExtractNestedTag(xml, "origin", "icao_code").value_or(std::string{});
    plan.destination = ExtractNestedTag(xml, "destination", "icao_code").value_or(std::string{});
    plan.generatedEpoch = ParseSimbriefPlanEpoch(xml);

    return plan;
}
