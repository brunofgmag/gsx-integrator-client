#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_ECHOWAIT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_ECHOWAIT_H

class EchoWait
{
public:
    explicit EchoWait(const int ticksToWait)
        : ticksToWait_(ticksToWait),
          ticksSinceWrite_(ticksToWait)
    {
    }

    [[nodiscard]] bool WriteIsDue(const bool echoed)
    {
        if (echoed)
        {
            ticksSinceWrite_ = ticksToWait_;

            return false;
        }

        if (ticksSinceWrite_ < ticksToWait_)
        {
            ++ticksSinceWrite_;

            return false;
        }

        ticksSinceWrite_ = 0;

        return true;
    }

private:
    int ticksToWait_;
    int ticksSinceWrite_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_ECHOWAIT_H
