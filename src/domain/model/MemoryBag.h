#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_MEMORYBAG_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_MEMORYBAG_H

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

class MemoryBag
{
public:
    using Entry = std::pair<std::string, std::string>;

    MemoryBag() = default;

    explicit MemoryBag(std::vector<Entry> entries)
        : entries_(std::move(entries))
    {
    }

    void PutFlag(const std::string& name, const bool value)
    {
        PutText(name, std::string(value ? kTrueText : kFalseText));
    }

    void PutNumber(const std::string& name, const double value)
    {
        std::array<char, kNumberBufferSize> buffer{};
        const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);

        PutText(name, std::string(buffer.data(), result.ptr));
    }

    void PutText(const std::string& name, std::string value)
    {
        const auto existing = FindEntry(name);
        if (existing != entries_.end())
        {
            existing->second = std::move(value);

            return;
        }

        entries_.emplace_back(name, std::move(value));
    }

    [[nodiscard]] bool Flag(const std::string_view name, const bool fallback) const
    {
        const auto entry = FindEntry(name);
        if (entry == entries_.end())
        {
            return fallback;
        }

        if (entry->second == kTrueText)
        {
            return true;
        }

        if (entry->second == kFalseText)
        {
            return false;
        }

        return fallback;
    }

    [[nodiscard]] double Number(const std::string_view name, const double fallback) const
    {
        const auto entry = FindEntry(name);
        if (entry == entries_.end())
        {
            return fallback;
        }

        const char* const first = entry->second.data();
        const char* const last = first + entry->second.size();
        double value = 0.0;
        const auto result = std::from_chars(first, last, value);

        return result.ec == std::errc{} && result.ptr == last && std::isfinite(value) ? value : fallback;
    }

    [[nodiscard]] std::string Text(const std::string_view name, std::string fallback) const
    {
        const auto entry = FindEntry(name);

        return entry == entries_.end() ? std::move(fallback) : entry->second;
    }

    [[nodiscard]] const std::vector<Entry>& Entries() const { return entries_; }

    bool operator==(const MemoryBag&) const = default;

private:
    static constexpr std::size_t kNumberBufferSize = 32;
    static constexpr std::string_view kTrueText = "1";
    static constexpr std::string_view kFalseText = "0";

    [[nodiscard]] std::vector<Entry>::iterator FindEntry(const std::string_view name)
    {
        return std::ranges::find_if(entries_, [name](const Entry& entry) { return entry.first == name; });
    }

    [[nodiscard]] std::vector<Entry>::const_iterator FindEntry(const std::string_view name) const
    {
        return std::ranges::find_if(entries_, [name](const Entry& entry) { return entry.first == name; });
    }

    std::vector<Entry> entries_;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_MEMORYBAG_H
