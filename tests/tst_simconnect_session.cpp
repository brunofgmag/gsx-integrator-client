#include <cstring>
#include <string>
#include <vector>

#include <QtTest/QTest>

#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/simconnect/SimConnectSession.h"

namespace
{
    constexpr DWORD kEvent1Sec = 1;
    constexpr DWORD kEvent4Sec = 2;
    constexpr DWORD kEventSimRunning = 3;
    constexpr DWORD kEvent6Hz = 4;
    constexpr DWORD kEventMenuToggle = 5;
    constexpr DWORD kEventPauseEx1 = 6;
    constexpr DWORD kRequestSimState = 0x0FFFFFFF;

    SIMCONNECT_RECV_EVENT MakeEvent(const DWORD eventId, const DWORD data)
    {
        SIMCONNECT_RECV_EVENT event{};
        event.uEventID = eventId;
        event.dwData = data;

        return event;
    }
}

class SimConnectSessionTest final : public QObject
{
    Q_OBJECT

private slots:
    static void init();

    static void openFailureLeavesDisconnected();
    static void subscribeRequiresConnection();
    static void subscribeRegistersSystemEventNames();
    static void subscribeFailureClearsCallback();
    static void dispatchRoutesTimerEvents();
    static void dispatchRoutesSimRunningFromEventAndSystemState();
    static void dispatchRoutesPauseAndMenuEvents();
    static void openEventDeliversApplicationName();
    static void quitEventFiresAfterDispatch();
    static void undersizedEventMessageIsIgnored();
    static void transmitRequiresConnection();
    static void closeClearsCallbacks();
};

void SimConnectSessionTest::init()
{
    FakeSimConnectApi::Reset();
}

void SimConnectSessionTest::openFailureLeavesDisconnected()
{
    FakeSimConnectApi::openSucceeds = false;

    SimConnectSession session;

    QVERIFY(!session.Open("test"));
    QVERIFY(!session.IsConnected());
}

void SimConnectSessionTest::subscribeRequiresConnection()
{
    SimConnectSession session;

    QVERIFY(!session.SubscribeOneSecond([] {}));
    QVERIFY(!session.SubscribeFourSeconds([] {}));
    QVERIFY(!session.SubscribeSixHz([] {}));
    QVERIFY(!session.SubscribeToPause([](unsigned) {}));
    QVERIFY(!session.SubscribeSimRunning([](bool) {}));
    QVERIFY(!session.SubscribeToExternalSystemToggle([](int) {}));
}

void SimConnectSessionTest::subscribeRegistersSystemEventNames()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    QVERIFY(session.SubscribeOneSecond([] {}));
    QVERIFY(session.SubscribeFourSeconds([] {}));
    QVERIFY(session.SubscribeSixHz([] {}));
    QVERIFY(session.SubscribeToPause([](unsigned) {}));
    QVERIFY(session.SubscribeSimRunning([](bool) {}));

    const std::vector<std::pair<DWORD, std::string>> expected = {
        {kEvent1Sec, "1sec"},
        {kEvent4Sec, "4sec"},
        {kEvent6Hz, "6Hz"},
        {kEventPauseEx1, "Pause_EX1"},
        {kEventSimRunning, "Sim"},
    };

    QCOMPARE(FakeSimConnectApi::systemEventSubscriptions, expected);
    QCOMPARE(FakeSimConnectApi::systemStateRequests, std::vector<std::string>{"Sim"});
}

void SimConnectSessionTest::subscribeFailureClearsCallback()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    int ticks = 0;
    FakeSimConnectApi::subscribeSucceeds = false;

    QVERIFY(!session.SubscribeOneSecond([&ticks] { ++ticks; }));

    FakeSimConnectApi::subscribeSucceeds = true;
    FakeSimConnectApi::Push(MakeEvent(kEvent1Sec, 0), SIMCONNECT_RECV_ID_EVENT);

    QVERIFY(session.Dispatch());

    QCOMPARE(ticks, 0);
}

void SimConnectSessionTest::dispatchRoutesTimerEvents()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    int oneSec = 0;
    int fourSec = 0;
    int sixHz = 0;

    QVERIFY(session.SubscribeOneSecond([&oneSec] { ++oneSec; }));
    QVERIFY(session.SubscribeFourSeconds([&fourSec] { ++fourSec; }));
    QVERIFY(session.SubscribeSixHz([&sixHz] { ++sixHz; }));

    FakeSimConnectApi::Push(MakeEvent(kEvent1Sec, 0), SIMCONNECT_RECV_ID_EVENT);
    FakeSimConnectApi::Push(MakeEvent(kEvent4Sec, 0), SIMCONNECT_RECV_ID_EVENT);
    FakeSimConnectApi::Push(MakeEvent(kEvent6Hz, 0), SIMCONNECT_RECV_ID_EVENT);

    QVERIFY(session.Dispatch());

    QCOMPARE(oneSec, 1);
    QCOMPARE(fourSec, 1);
    QCOMPARE(sixHz, 1);
}

void SimConnectSessionTest::dispatchRoutesSimRunningFromEventAndSystemState()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    std::vector<bool> running;

    QVERIFY(session.SubscribeSimRunning([&running](const bool value) { running.push_back(value); }));

    FakeSimConnectApi::Push(MakeEvent(kEventSimRunning, 1), SIMCONNECT_RECV_ID_EVENT);

    SIMCONNECT_RECV_SYSTEM_STATE state{};
    state.dwRequestID = kRequestSimState;
    state.dwInteger = 0;
    FakeSimConnectApi::Push(state, SIMCONNECT_RECV_ID_SYSTEM_STATE);

    QVERIFY(session.Dispatch());

    QCOMPARE(running, (std::vector<bool>{true, false}));
}

void SimConnectSessionTest::dispatchRoutesPauseAndMenuEvents()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    unsigned pauseState = 99;
    int menuState = -1;

    QVERIFY(session.SubscribeToPause([&pauseState](const unsigned value) { pauseState = value; }));
    QVERIFY(session.SubscribeToExternalSystemToggle([&menuState](const int value) { menuState = value; }));

    FakeSimConnectApi::Push(MakeEvent(kEventPauseEx1, 2), SIMCONNECT_RECV_ID_EVENT);
    FakeSimConnectApi::Push(MakeEvent(kEventMenuToggle, 7), SIMCONNECT_RECV_ID_EVENT);

    QVERIFY(session.Dispatch());

    QCOMPARE(pauseState, 2u);
    QCOMPARE(menuState, 7);
}

void SimConnectSessionTest::openEventDeliversApplicationName()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    std::string appName;
    session.SetOnOpen([&appName](const char* name) { appName = name; });

    SIMCONNECT_RECV_OPEN open{};
    strcpy_s(open.szApplicationName, "Microsoft Flight Simulator");
    FakeSimConnectApi::Push(open, SIMCONNECT_RECV_ID_OPEN);

    QVERIFY(session.Dispatch());

    QCOMPARE(appName, std::string("Microsoft Flight Simulator"));
}

void SimConnectSessionTest::quitEventFiresAfterDispatch()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    int quits = 0;
    session.SetOnQuit([&quits] { ++quits; });

    constexpr SIMCONNECT_RECV quit{};
    FakeSimConnectApi::Push(quit, SIMCONNECT_RECV_ID_QUIT);

    QVERIFY(session.Dispatch());

    QCOMPARE(quits, 1);

    QVERIFY(session.Dispatch());
    QCOMPARE(quits, 1);
}

void SimConnectSessionTest::undersizedEventMessageIsIgnored()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    int ticks = 0;

    QVERIFY(session.SubscribeOneSecond([&ticks] { ++ticks; }));

    FakeSimConnectApi::PushTruncated(MakeEvent(kEvent1Sec, 0), SIMCONNECT_RECV_ID_EVENT,
                                     sizeof(SIMCONNECT_RECV));
    QVERIFY(session.Dispatch());

    QCOMPARE(ticks, 0);
}

void SimConnectSessionTest::transmitRequiresConnection()
{
    SimConnectSession session;

    QVERIFY(!session.TransmitExternalSystemToggle(1));
    QCOMPARE(FakeSimConnectApi::transmittedEvents, 0);

    QVERIFY(session.Open("test"));
    QVERIFY(session.TransmitExternalSystemToggle(1));
    QCOMPARE(FakeSimConnectApi::transmittedEvents, 1);
}

void SimConnectSessionTest::closeClearsCallbacks()
{
    SimConnectSession session;

    QVERIFY(session.Open("test"));

    int ticks = 0;

    QVERIFY(session.SubscribeOneSecond([&ticks] { ++ticks; }));

    session.Close();

    QVERIFY(session.Open("test"));

    FakeSimConnectApi::Push(MakeEvent(kEvent1Sec, 0), SIMCONNECT_RECV_ID_EVENT);

    QVERIFY(session.Dispatch());

    QCOMPARE(ticks, 0);
}

QTEST_GUILESS_MAIN(SimConnectSessionTest)

#include "tst_simconnect_session.moc"
