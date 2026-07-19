#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMCONNECTAPI_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMCONNECTAPI_H

#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>
#include <SimConnect.h>

struct FakeSimConnectApi
{
    static inline bool openSucceeds = true;
    static inline bool subscribeSucceeds = true;
    static inline std::vector<std::pair<DWORD, std::string>> systemEventSubscriptions;
    static inline std::vector<std::string> systemStateRequests;
    static inline int transmittedEvents = 0;
    static inline std::vector<std::vector<char>> pendingMessages;

    static void Reset()
    {
        openSucceeds = true;
        subscribeSucceeds = true;
        systemEventSubscriptions.clear();
        systemStateRequests.clear();
        transmittedEvents = 0;
        pendingMessages.clear();
    }

    template <typename T>
    static void Push(T message, const DWORD id)
    {
        message.dwSize = sizeof(T);
        message.dwVersion = 0;
        message.dwID = id;

        std::vector<char> bytes(sizeof(T));
        std::memcpy(bytes.data(), &message, sizeof(T));
        pendingMessages.push_back(std::move(bytes));
    }

    template <typename T>
    static void PushTruncated(T message, const DWORD id, const std::size_t truncatedSize)
    {
        message.dwSize = sizeof(T);
        message.dwVersion = 0;
        message.dwID = id;

        std::vector<char> bytes(sizeof(T));
        std::memcpy(bytes.data(), &message, sizeof(T));
        bytes.resize(truncatedSize);
        pendingMessages.push_back(std::move(bytes));
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMCONNECTAPI_H
