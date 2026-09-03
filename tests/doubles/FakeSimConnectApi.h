#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMCONNECTAPI_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMCONNECTAPI_H

#include <cstddef>
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
    static inline std::vector<std::string> mappedClientDataAreas;
    static inline std::vector<std::string> mappedEventNames;
    static inline std::vector<std::pair<DWORD, std::string>> transmittedNamedEvents;
    static inline std::vector<std::vector<char>> writtenClientData;

    struct DataRequest
    {
        DWORD defineId = 0;
        SIMCONNECT_PERIOD period = SIMCONNECT_PERIOD_NEVER;
        DWORD interval = 0;
    };

    static inline std::vector<DataRequest> dataRequests;
    static inline std::vector<std::pair<DWORD, std::string>> dataDefinitions;

    static void Reset()
    {
        openSucceeds = true;
        subscribeSucceeds = true;
        systemEventSubscriptions.clear();
        systemStateRequests.clear();
        transmittedEvents = 0;
        pendingMessages.clear();
        mappedClientDataAreas.clear();
        mappedEventNames.clear();
        transmittedNamedEvents.clear();
        writtenClientData.clear();
        dataRequests.clear();
        dataDefinitions.clear();
    }

    static DWORD DefineIdOf(const std::string& datumName)
    {
        for (const auto& [defineId, name] : dataDefinitions)
        {
            if (name == datumName)
            {
                return defineId;
            }
        }

        return 0;
    }

    static void PushSimObjectDouble(const DWORD requestId, const double value)
    {
        std::vector<char> bytes(sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) + sizeof(double), 0);
        const auto data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(bytes.data());
        data->dwSize = static_cast<DWORD>(bytes.size());
        data->dwVersion = 0;
        data->dwID = SIMCONNECT_RECV_ID_SIMOBJECT_DATA;
        data->dwRequestID = requestId;
        data->dwDefineID = requestId;
        std::memcpy(&data->dwData, &value, sizeof(double));
        pendingMessages.push_back(std::move(bytes));
    }

    static void PushClientData(const DWORD requestId, const void* payload, const std::size_t payloadSize)
    {
        SIMCONNECT_RECV_CLIENT_DATA header{};
        header.dwSize = static_cast<DWORD>(offsetof(SIMCONNECT_RECV_CLIENT_DATA, dwData) + payloadSize);
        header.dwVersion = 0;
        header.dwID = SIMCONNECT_RECV_ID_CLIENT_DATA;
        header.dwRequestID = requestId;

        std::vector<char> bytes(offsetof(SIMCONNECT_RECV_CLIENT_DATA, dwData) + payloadSize);
        std::memcpy(bytes.data(), &header, offsetof(SIMCONNECT_RECV_CLIENT_DATA, dwData));
        if (payloadSize > 0 && payload != nullptr)
        {
            std::memcpy(bytes.data() + offsetof(SIMCONNECT_RECV_CLIENT_DATA, dwData), payload, payloadSize);
        }

        pendingMessages.push_back(std::move(bytes));
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
