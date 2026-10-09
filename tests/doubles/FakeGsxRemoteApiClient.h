#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H

#include <string>
#include <vector>

class GsxRemoteApiClient;
class QJsonObject;

struct FakeGsxRemoteApi
{
    static inline int startCalls = 0;
    static inline int stopCalls = 0;
    static inline std::vector<std::string> commandVerbs;
    static inline GsxRemoteApiClient* liveClient = nullptr;
    static inline bool connectionUp = false;

    static void Reset()
    {
        startCalls = 0;
        stopCalls = 0;
        commandVerbs.clear();
        connectionUp = false;
    }

    static void AnnounceConnection(bool connected);
    static void Receive(const QJsonObject& message);
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXREMOTEAPICLIENT_H
