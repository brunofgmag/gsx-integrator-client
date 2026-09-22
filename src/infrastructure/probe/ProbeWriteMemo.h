#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWRITEMEMO_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWRITEMEMO_H

#include <optional>
#include <string>
#include <unordered_map>
#include <QtCore/QString>

namespace probe
{
    inline constexpr long long kWriteRepeatAfterMs = 10000;

    class WriteMemo
    {
    public:
        [[nodiscard]] std::optional<int> Record(const std::string& name, const QString& value, const long long nowMs)
        {
            Entry& entry = entries_[name];
            ++entry.count;

            if (entry.logged && entry.value == value && nowMs - entry.loggedAtMs < kWriteRepeatAfterMs)
            {
                return std::nullopt;
            }

            entry.value = value;
            entry.loggedAtMs = nowMs;
            entry.logged = true;

            return entry.count;
        }

    private:
        struct Entry
        {
            QString value;
            long long loggedAtMs = 0;
            int count = 0;
            bool logged = false;
        };

        std::unordered_map<std::string, Entry> entries_;
    };
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBEWRITEMEMO_H
