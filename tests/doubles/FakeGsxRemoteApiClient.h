#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H

#include <string>
#include <vector>

struct FakeGsxRemoteApi
{
    static inline int startCalls = 0;
    static inline int stopCalls = 0;
    static inline std::vector<std::string> commandVerbs;

    static void Reset()
    {
        startCalls = 0;
        stopCalls = 0;
        commandVerbs.clear();
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
