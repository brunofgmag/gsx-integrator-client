#include "AircraftRegistry.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{
    constexpr int kTitleScore = 2;
    constexpr int kAtcModelScore = 4;

    std::string ToLower(const std::string& text)
    {
        std::string lowered = text;
        std::ranges::transform(lowered, lowered.begin(),
                               [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lowered;
    }

    int ScoreFor(const MatchField field)
    {
        return field == MatchField::AtcModel ? kAtcModelScore : kTitleScore;
    }

    const std::string& ValueFor(const AircraftIdentity& identity, const MatchField field)
    {
        return field == MatchField::AtcModel ? identity.atcModel : identity.title;
    }

    int Score(const AircraftDescriptor& descriptor, const AircraftIdentity& identity)
    {
        int score = 0;
        for (const MatchField field : {MatchField::Title, MatchField::AtcModel})
        {
            for (const MatchRule& rule : descriptor.rules)
            {
                if (rule.field == field && MatchText(ValueFor(identity, field), rule.op, rule.pattern))
                {
                    score += ScoreFor(field);
                    break;
                }
            }
        }
        return score;
    }
}

std::vector<const AircraftDescriptor*>& AircraftRegistry()
{
    static std::vector<const AircraftDescriptor*> registry;
    return registry;
}

AircraftRegistration::AircraftRegistration(const AircraftDescriptor& descriptor)
{
    AircraftRegistry().push_back(&descriptor);
}

bool MatchText(const std::string& value, const MatchOp op, const char* pattern)
{
    const std::string loweredPattern = ToLower(pattern);
    if (loweredPattern.empty())
    {
        return false;
    }

    const std::string loweredValue = ToLower(value);
    switch (op)
    {
    case MatchOp::Equals:
        return loweredValue == loweredPattern;
    case MatchOp::StartsWith:
        return loweredValue.rfind(loweredPattern, 0) == 0;
    case MatchOp::Contains:
        return loweredValue.find(loweredPattern) != std::string::npos;
    }
    return false;
}

const AircraftDescriptor* MatchAircraft(
    const std::vector<const AircraftDescriptor*>& candidates,
    const AircraftIdentity& identity)
{
    const AircraftDescriptor* best = nullptr;
    int bestScore = 0;

    for (const AircraftDescriptor* candidate : candidates)
    {
        const int score = Score(*candidate, identity);
        if (score == 0)
        {
            continue;
        }

        if (best == nullptr
            || score > bestScore
            || (score == bestScore && std::strcmp(candidate->name, best->name) < 0))
        {
            best = candidate;
            bestScore = score;
        }
    }

    return best;
}
