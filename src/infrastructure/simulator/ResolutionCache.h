#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RESOLUTIONCACHE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RESOLUTIONCACHE_H

#include <chrono>
#include <functional>
#include <mutex>
#include <QtCore/QString>

class ResolutionCache
{
public:
    using Clock = std::chrono::steady_clock;

    static constexpr Clock::duration kEmptyLifetime = std::chrono::seconds(5);

    [[nodiscard]] QString Resolve(const QString& key, const std::function<QString()>& resolve,
                                  const Clock::time_point now = Clock::now())
    {
        const std::scoped_lock lock(mutex_);
        if (filled_ && key == key_ && (!value_.isEmpty() || now - resolvedAt_ < kEmptyLifetime))
        {
            return value_;
        }

        key_ = key;
        value_ = resolve();
        resolvedAt_ = now;
        filled_ = true;

        return value_;
    }

    void Reset()
    {
        const std::scoped_lock lock(mutex_);
        filled_ = false;
    }

private:
    std::mutex mutex_;
    bool filled_ = false;
    QString key_;
    QString value_;
    Clock::time_point resolvedAt_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RESOLUTIONCACHE_H
