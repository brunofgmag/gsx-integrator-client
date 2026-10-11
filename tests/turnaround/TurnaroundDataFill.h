#ifndef GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDDATAFILL_H
#define GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDDATAFILL_H

#include <optional>
#include <set>
#include <string>
#include <type_traits>

#include "src/domain/turnaround/TurnaroundData.h"

template <typename Value>
void MakeNonDefault(Value& value)
{
    if constexpr (std::is_same_v<Value, bool>)
    {
        value = true;
    }
    else if constexpr (std::is_same_v<Value, int>)
    {
        value = 7;
    }
    else if constexpr (std::is_same_v<Value, double>)
    {
        value = 7.5;
    }
    else if constexpr (std::is_enum_v<Value>)
    {
        value = static_cast<Value>(1);
    }
    else if constexpr (std::is_same_v<Value, std::optional<int>>)
    {
        value = 3;
    }
    else if constexpr (std::is_same_v<Value, std::set<std::string>>)
    {
        value.insert("a-rule");
    }
    else
    {
        static_assert(sizeof(Value) == 0, "teach MakeNonDefault the type of the new TurnaroundData field");
    }
}

inline TurnaroundData EveryFieldNonDefault()
{
    TurnaroundData data;
    turnaround::VisitFields(
        [&data](const char*, const auto accessor, const turnaround::FieldRestore)
        {
            MakeNonDefault(accessor(data));
        });

    return data;
}

#endif // GSX_INTEGRATOR_CLIENT_TESTS_TURNAROUNDDATAFILL_H
