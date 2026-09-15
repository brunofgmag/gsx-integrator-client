#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H

#include <string>
#include <vector>

class GsxRemoteApiClient;

struct FakeGsxRemoteApi
{
    static inline int startCalls = 0;
    static inline int stopCalls = 0;
    static inline std::vector<std::string> commandVerbs;
    static inline GsxRemoteApiClient* liveClient = nullptr;

    static void Reset()
    {
        startCalls = 0;
        stopCalls = 0;
        commandVerbs.clear();
    }

    static void AnnounceConnection(bool connected);
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
