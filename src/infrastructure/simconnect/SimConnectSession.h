#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTSESSION_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTSESSION_H

#include <functional>
#include <utility>
#include <windows.h>
#include <SimConnect.h>

class SimConnectVariableGateway;

class SimConnectSession
{
public:
    using MenuEventFn = std::function<void(int)>;
    using EventFn = std::function<void()>;
    using SimRunningFn = std::function<void(bool)>;
    using PauseFn = std::function<void(unsigned)>;
    using OpenFn = std::function<void(const char*)>;

    bool Open(const char* name);
    void Close();

    [[nodiscard]] bool IsConnected() const { return hSimConnect_ != nullptr; }
    [[nodiscard]] HANDLE Handle() const { return hSimConnect_; }

    bool TransmitExternalSystemToggle(int state) const;

    bool SubscribeToPause(PauseFn onPause);
    bool SubscribeToExternalSystemToggle(MenuEventFn onMenuEvent);
    bool SubscribeOneSecond(EventFn onOneSecond);
    bool SubscribeFourSeconds(EventFn onFourSeconds);
    bool SubscribeSixHz(EventFn onSixHz);
    bool SubscribeSimRunning(SimRunningFn onSimRunning);

    void SetOnOpen(OpenFn onOpen) { onOpen_ = std::move(onOpen); }
    void SetOnQuit(EventFn onQuit) { onQuit_ = std::move(onQuit); }
    void SetVarManager(SimConnectVariableGateway* varManager) { varManager_ = varManager; }

    bool Dispatch();

private:
    enum : SIMCONNECT_CLIENT_EVENT_ID
    {
        kEvent1Sec = 1,
        kEvent4Sec = 2,
        kEventSimRunning = 3,
        kEvent6Hz = 4,
        kEventMenuToggle = 5,
        kEventPauseEx1 = 6,
    };

    enum : SIMCONNECT_NOTIFICATION_GROUP_ID
    {
        kGroupMenu = 1,
    };

    static constexpr DWORD kRequestSimState = 0x0FFFFFFF;

    static void CALLBACK DispatchTrampoline(SIMCONNECT_RECV* pData, DWORD cbData, void* ctx);
    void HandleMessage(SIMCONNECT_RECV* pData, DWORD cbData);
    void HandleOpen(const SIMCONNECT_RECV* pData, DWORD cbData) const;
    void HandleEvent(const SIMCONNECT_RECV* pData, DWORD cbData) const;
    void HandleSystemState(const SIMCONNECT_RECV* pData, DWORD cbData) const;

    template <typename Fn>
    bool SubscribeSystemEvent(Fn& target, Fn fn, SIMCONNECT_CLIENT_EVENT_ID eventId, const char* name);

    HANDLE hSimConnect_ = nullptr;
    SimConnectVariableGateway* varManager_ = nullptr;
    bool quitPending_ = false;
    OpenFn onOpen_ = nullptr;
    PauseFn onPause_ = nullptr;
    MenuEventFn onMenuEvent_;
    EventFn onOneSecond_;
    EventFn onFourSeconds_;
    EventFn onSixHz_;
    EventFn onQuit_;
    SimRunningFn onSimRunning_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTSESSION_H
