#include "SimConnectSession.h"

#include <utility>
#include "SimConnectVariableGateway.h"
#include "../logging/LogMacros.h"

bool SimConnectSession::Open(const char* name)
{
    Close();

    if (const HRESULT hr = SimConnect_Open(&hSimConnect_, name, nullptr, 0, nullptr, 0); FAILED(hr))
    {
        hSimConnect_ = nullptr;
        return false;
    }

    return true;
}

void SimConnectSession::Close()
{
    if (hSimConnect_ != nullptr)
    {
        if (const HRESULT hr = SimConnect_Close(hSimConnect_); FAILED(hr))
        {
            LOG_ERROR("Failed to close SimConnect connection: Err code %i", static_cast<int>(hr));
        }
        hSimConnect_ = nullptr;
    }

    onOneSecond_ = nullptr;
    onFourSeconds_ = nullptr;
    onSixHz_ = nullptr;
    onSimRunning_ = nullptr;
    onQuit_ = nullptr;
    onPause_ = nullptr;
    onMenuEvent_ = nullptr;
    onOpen_ = nullptr;
}

bool SimConnectSession::TransmitExternalSystemToggle(const int state) const
{
    if (!IsConnected())
    {
        return false;
    }

    const HRESULT hr = SimConnect_TransmitClientEvent(hSimConnect_,
                                                      SIMCONNECT_OBJECT_ID_USER,
                                                      kEventMenuToggle,
                                                      static_cast<DWORD>(state),
                                                      kGroupMenu,
                                                      SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    if (FAILED(hr))
    {
        LOG_WARN("Failed to transmit EXTERNAL_SYSTEM_TOGGLE=%d: hr=%i", state, static_cast<int>(hr));

        return false;
    }

    return true;
}

template <typename Fn>
bool SimConnectSession::SubscribeSystemEvent(Fn& target, Fn fn, const SIMCONNECT_CLIENT_EVENT_ID eventId,
                                             const char* name)
{
    if (!IsConnected())
    {
        return false;
    }

    target = std::move(fn);

    if (FAILED(SimConnect_SubscribeToSystemEvent(hSimConnect_, eventId, name)))
    {
        target = nullptr;

        return false;
    }

    return true;
}

bool SimConnectSession::SubscribeToPause(PauseFn onPause)
{
    return SubscribeSystemEvent(onPause_, std::move(onPause), kEventPauseEx1, "Pause_EX1");
}

bool SimConnectSession::SubscribeToExternalSystemToggle(MenuEventFn onMenuEvent)
{
    if (!IsConnected())
    {
        return false;
    }

    onMenuEvent_ = std::move(onMenuEvent);

    const bool mapped =
        SUCCEEDED(SimConnect_MapClientEventToSimEvent(hSimConnect_, kEventMenuToggle, "EXTERNAL_SYSTEM_TOGGLE"))
        && SUCCEEDED(SimConnect_AddClientEventToNotificationGroup(hSimConnect_, kGroupMenu, kEventMenuToggle))
        && SUCCEEDED(SimConnect_SetNotificationGroupPriority(hSimConnect_, kGroupMenu,
                                                             SIMCONNECT_GROUP_PRIORITY_HIGHEST));
    if (!mapped)
    {
        onMenuEvent_ = nullptr;

        return false;
    }

    return true;
}

bool SimConnectSession::SubscribeOneSecond(EventFn onOneSecond)
{
    return SubscribeSystemEvent(onOneSecond_, std::move(onOneSecond), kEvent1Sec, "1sec");
}

bool SimConnectSession::SubscribeFourSeconds(EventFn onFourSeconds)
{
    return SubscribeSystemEvent(onFourSeconds_, std::move(onFourSeconds), kEvent4Sec, "4sec");
}

bool SimConnectSession::SubscribeSixHz(EventFn onSixHz)
{
    return SubscribeSystemEvent(onSixHz_, std::move(onSixHz), kEvent6Hz, "6Hz");
}

bool SimConnectSession::SubscribeSimRunning(SimRunningFn onSimRunning)
{
    if (!SubscribeSystemEvent(onSimRunning_, std::move(onSimRunning), kEventSimRunning, "Sim"))
    {
        return false;
    }

    if (const HRESULT hr = SimConnect_RequestSystemState(hSimConnect_, kRequestSimState, "Sim"); FAILED(hr))
    {
        LOG_WARN("Failed to request current 'Sim' system state: Err code %i", static_cast<int>(hr));
    }

    return true;
}

bool SimConnectSession::Dispatch()
{
    if (!IsConnected())
    {
        return false;
    }

    const HRESULT hr = SimConnect_CallDispatch(hSimConnect_, &DispatchTrampoline, this);

    if (quitPending_)
    {
        quitPending_ = false;
        if (onQuit_)
        {
            const EventFn quit = onQuit_;
            quit();
        }
    }

    return SUCCEEDED(hr);
}

void CALLBACK SimConnectSession::DispatchTrampoline(SIMCONNECT_RECV* pData, const DWORD cbData, void* ctx)
{
    if (!ctx || !pData)
    {
        return;
    }

    static_cast<SimConnectSession*>(ctx)->HandleMessage(pData, cbData);
}

void SimConnectSession::HandleMessage(SIMCONNECT_RECV* pData, const DWORD cbData)
{
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_OPEN:
        HandleOpen(pData, cbData);
        break;

    case SIMCONNECT_RECV_ID_EVENT:
        HandleEvent(pData, cbData);
        break;

    case SIMCONNECT_RECV_ID_SYSTEM_STATE:
        HandleSystemState(pData, cbData);
        break;

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        if (varManager_)
        {
            varManager_->HandleSimObjectData(static_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA*>(pData));
        }
        break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
        {
            const auto* ex = static_cast<const SIMCONNECT_RECV_EXCEPTION*>(pData);
            LOG_WARN("SimConnect exception %u (sendId=%u, index=%u)",
                     static_cast<unsigned>(ex->dwException),
                     static_cast<unsigned>(ex->dwSendID),
                     static_cast<unsigned>(ex->dwIndex));
            break;
        }

    case SIMCONNECT_RECV_ID_QUIT:
        LOG_INFO("Simulator quit notification received.");
        quitPending_ = true;
        break;

    default:
        break;
    }
}

void SimConnectSession::HandleOpen(const SIMCONNECT_RECV* pData, const DWORD cbData) const
{
    if (cbData < sizeof(SIMCONNECT_RECV_OPEN))
    {
        return;
    }

    const auto* openData = static_cast<const SIMCONNECT_RECV_OPEN*>(pData);
    if (onOpen_)
    {
        onOpen_(openData->szApplicationName);
    }
}

void SimConnectSession::HandleEvent(const SIMCONNECT_RECV* pData, const DWORD cbData) const
{
    if (cbData < sizeof(SIMCONNECT_RECV_EVENT))
    {
        return;
    }

    const auto* eventData = static_cast<const SIMCONNECT_RECV_EVENT*>(pData);
    if (eventData->uEventID == kEvent1Sec && onOneSecond_)
    {
        onOneSecond_();
    }
    else if (eventData->uEventID == kEvent4Sec && onFourSeconds_)
    {
        onFourSeconds_();
    }
    else if (eventData->uEventID == kEvent6Hz && onSixHz_)
    {
        onSixHz_();
    }
    else if (eventData->uEventID == kEventSimRunning && onSimRunning_)
    {
        onSimRunning_(eventData->dwData >= 1);
    }
    else if (eventData->uEventID == kEventMenuToggle && onMenuEvent_)
    {
        onMenuEvent_(static_cast<int>(eventData->dwData));
    }
    else if (eventData->uEventID == kEventPauseEx1 && onPause_)
    {
        onPause_(static_cast<int>(eventData->dwData));
    }
}

void SimConnectSession::HandleSystemState(const SIMCONNECT_RECV* pData, const DWORD cbData) const
{
    if (cbData < sizeof(SIMCONNECT_RECV_SYSTEM_STATE))
    {
        return;
    }

    const auto* stateData = static_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(pData);
    if (stateData->dwRequestID == kRequestSimState && onSimRunning_)
    {
        onSimRunning_(stateData->dwInteger >= 1);
    }
}
