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

    std::optional<double> ParseWeightKg(const std::string_view xml, const std::string_view tag, const bool pounds)
    {
        const auto text = ExtractTag(xml, tag);
        if (!text)
        {
            return std::nullopt;
        }

        const auto parsed = ParseDouble(*text);
        if (!parsed || *parsed < 0.0)
        {
            return std::nullopt;
        }

        return pounds ? weight::LbToKg(*parsed) : *parsed;
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

    const auto units = ExtractTag(xml, "units");
    const bool ofpInPounds = units && *units == "lbs";
    const auto unit = ofpInPounds ? WeightUnit::Lb : WeightUnit::Kg;

    const auto fuelKg = ParseWeightKg(xml, "plan_ramp", ofpInPounds);
    const auto zfwKg = ParseWeightKg(xml, "est_zfw", ofpInPounds);
    if (!fuelKg || !zfwKg)
    {
        return std::nullopt;
    }

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

    if (*fuelKg <= 0.0 || *zfwKg <= 0.0 || passengers < 0)
    {
        return std::nullopt;
    }

    FlightPlan plan{*fuelKg, *zfwKg, passengers, unit};
    plan.origin = ExtractNestedTag(xml, "origin", "icao_code").value_or(std::string{});
    plan.destination = ExtractNestedTag(xml, "destination", "icao_code").value_or(std::string{});
    plan.generatedEpoch = ParseSimbriefPlanEpoch(xml);
    plan.operatingEmptyKg = ParseWeightKg(xml, "oew", ofpInPounds).value_or(0.0);
    plan.payloadKg = ParseWeightKg(xml, "payload", ofpInPounds);
    plan.cargoKg = ParseWeightKg(xml, "cargo", ofpInPounds);

    return plan;
}
