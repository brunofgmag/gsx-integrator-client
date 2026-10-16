#include <QtTest/QTest>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "ProbeLines.h"
#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/simconnect/SimConnectApi.h"
#include "../src/infrastructure/simconnect/SimConnectVariableGateway.h"

namespace
{
    constexpr auto kEng1N1 = "md11_eng1_n1";
    constexpr auto kEng3N1 = "md11_eng3_n1";
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kKgUnit = "kg";
    constexpr auto kGpuAvail = "FSS_B727_GPU_AVAIL";
    constexpr auto kPayloadStation = "PAYLOAD STATION WEIGHT:1";
    constexpr auto kPoundsUnit = "pounds";
    constexpr auto kWritesLog = "writes.log";
    constexpr DWORD kFirstDefineId = 1;
    constexpr std::size_t kString256 = 256;
    constexpr auto kSuper27FreighterTitle = "Boeing 727-200RE Super 27 Freighter";
    constexpr auto kSuper27AtcModel = "B727RE";
    constexpr auto kUnitTypo = "gallonz";
    constexpr DWORD kUnrecognizedId = SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID;
    constexpr DWORD kNameUnrecognized = SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED;
    constexpr DWORD kIllegalOperation = 25;
    constexpr long long kFirstRetryMs = 1000;
    constexpr long long kLongestWaitMs = 16000;
    constexpr unsigned kRetryBudget = 30;

    struct Rejections
    {
        long long nowMs = 0;
        SimConnectVariableGateway gateway;

        Rejections()
        {
            FakeSimConnectApi::Reset();
            gateway.SetClock([this] { return nowMs; });
            gateway.Attach(reinterpret_cast<HANDLE>(0x5150));
        }

        void Reject(const DWORD exception, const DWORD sendId)
        {
            SIMCONNECT_RECV_EXCEPTION message{};
            message.dwException = exception;
            message.dwSendID = sendId;
            gateway.HandleException(&message);
        }

        void RejectLastRequest(const DWORD exception = kUnrecognizedId)
        {
            Reject(exception, FakeSimConnectApi::dataRequests.back().sendId);
        }

        void Wait(const long long ms)
        {
            nowMs += ms;
            gateway.RetryRejected();
        }

        [[nodiscard]] std::size_t Requests() const
        {
            return FakeSimConnectApi::DataRequestsFor(kFirstDefineId);
        }
    };

    void DeliverDouble(SimConnectVariableGateway& gateway, const DWORD requestId, const double value)
    {
        std::vector<BYTE> buffer(sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) + sizeof(double), 0);
        const auto data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(buffer.data());
        data->dwRequestID = requestId;
        std::memcpy(&data->dwData, &value, sizeof(double));
        gateway.HandleSimObjectData(data);
    }

    void DeliverText(SimConnectVariableGateway& gateway, const DWORD requestId, const std::string& text)
    {
        std::vector<BYTE> buffer(sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) + kString256, 0);
        const auto data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(buffer.data());
        data->dwRequestID = requestId;
        std::memcpy(&data->dwData, text.c_str(), (std::min)(text.size(), kString256 - 1));
        gateway.HandleSimObjectData(data);
    }
}

class VariableGatewayTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    static void lvarReturnsDefaultUntilDataArrives();
    static void lvarReturnsRealValueAfterDataArrives();
    static void lvarHonorsCustomDefault();
    static void dataForOneSlotDoesNotLeakToAnother();
    static void detachResetsReceivedState();
    static void avarReportsReceivedOnlyAfterDataArrives();
    static void lvarReportsReceivedOnlyAfterDataArrives();
    static void fastRefreshSharesSlotWithGetLVar();
    static void fastRefreshPromotesAlreadyReadSlotToFrameRate();
    static void fastRefreshKeepsFrameRateWhenAskedTwice();
    static void consumeLVarSpanCatchesTransient();
    static void everyAskerSeesTheSameChangeInOneTick();
    static void theSpanPairAnswersTheSecondAskerDifferently();
    static void aVariableWithoutABaselineCountsAsChanged();
    static void detachForgetsTheTickBaseline();
    static void forgettingTheTextSlotsDropsThePreviousFlightStrings();
    static void forgettingTheTextSlotsLeavesTheNumbersAlone();
    static void forgettingTheTextSlotsAsksTheSimForTheStringsAgain();
    static void forgettingTheTextSlotsRebuildsTheStringDefinitionInsteadOfAppendingToIt();
    static void everyWriteToTheSimLandsInTheWritesLog();
    static void aRepeatedWriteIsCountedAndLoggedOnceUntilTheValueChanges();
    static void aWriteThatNeverLeftIsNotLogged();

    static void aRejectedRequestKeepsTheDefaultUntilItIsAskedAgainAndAnswered();
    static void aRejectedRequestIsNotAskedAgainBeforeTheBackoffPasses();
    static void theBackoffDoublesUpToACeiling();
    static void aRejectedAircraftTitleIsAskedAgain();
    static void aRetryRebuildsTheDefinitionInsteadOfAppendingToIt();
    static void aRejectedDefinitionOfAnLVarIsAskedAgainAndTheFollowUpRejectionIsAbsorbed();
    static void aRejectedDefinitionOfAnAVarIsNotAskedAgain();
    static void anExceptionThatIsNotATimingProblemIsNotAskedAgain_data();
    static void anExceptionThatIsNotATimingProblemIsNotAskedAgain();
    static void theRetriesStopAtTheBudget();
    static void reconnectingGivesTheBudgetBack();
    static void anExceptionForAnotherCallIsIgnored();
    static void anExceptionAfterTheDataArrivedIsIgnored();
    static void anExceptionWithoutASendIdIsNeverAttributedToAVariable();
    static void detachingCancelsThePendingRetry();
    static void aRetryThatCouldNotBeSentIsScheduledAgain();

private:
    QTemporaryDir directory_;
};

void VariableGatewayTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void VariableGatewayTest::lvarReturnsDefaultUntilDataArrives()
{
    SimConnectVariableGateway gateway;

    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);
}

void VariableGatewayTest::lvarReturnsRealValueAfterDataArrives()
{
    SimConnectVariableGateway gateway;
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);

    DeliverDouble(gateway, kFirstDefineId, 84.5);

    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 84.5);
}

void VariableGatewayTest::lvarHonorsCustomDefault()
{
    SimConnectVariableGateway gateway;
    QCOMPARE(gateway.GetLVar(kEng3N1, -1.0), -1.0);

    DeliverDouble(gateway, kFirstDefineId, 0.0);

    QCOMPARE(gateway.GetLVar(kEng3N1, -1.0), 0.0);
}

void VariableGatewayTest::dataForOneSlotDoesNotLeakToAnother()
{
    SimConnectVariableGateway gateway;
    gateway.GetLVar(kEng1N1, 0.0);
    gateway.GetLVar(kEng3N1, 0.0);

    DeliverDouble(gateway, kFirstDefineId + 1, 42.0);

    QCOMPARE(gateway.GetLVar(kEng1N1, 0.0), 0.0);
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 42.0);
}

void VariableGatewayTest::detachResetsReceivedState()
{
    SimConnectVariableGateway gateway;
    gateway.GetLVar(kEng3N1, 0.0);
    DeliverDouble(gateway, kFirstDefineId, 84.5);
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 84.5);

    gateway.Detach();

    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);
}

void VariableGatewayTest::avarReportsReceivedOnlyAfterDataArrives()
{
    SimConnectVariableGateway gateway;
    QVERIFY(!gateway.HasReceivedAVar(kSimFuelTotalKg, kKgUnit));

    DeliverDouble(gateway, kFirstDefineId, 18500.0);

    QVERIFY(gateway.HasReceivedAVar(kSimFuelTotalKg, kKgUnit));
    QCOMPARE(gateway.GetAVar(kSimFuelTotalKg, kKgUnit, 0.0), 18500.0);
}

void VariableGatewayTest::lvarReportsReceivedOnlyAfterDataArrives()
{
    SimConnectVariableGateway gateway;
    QVERIFY(!gateway.HasReceivedLVar(kEng3N1));

    DeliverDouble(gateway, kFirstDefineId, 25.166);

    QVERIFY(gateway.HasReceivedLVar(kEng3N1));
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 25.166);

    gateway.Detach();

    QVERIFY(!gateway.HasReceivedLVar(kEng3N1));
}

void VariableGatewayTest::consumeLVarSpanCatchesTransient()
{
    SimConnectVariableGateway gateway;
    QVERIFY(!gateway.ConsumeLVarSpan(kEng3N1).received);

    DeliverDouble(gateway, kFirstDefineId, 100.0);
    DeliverDouble(gateway, kFirstDefineId, 50.0);

    const LVarSpan up = gateway.ConsumeLVarSpan(kEng3N1);
    QCOMPARE(up.max, 100.0);
    QCOMPARE(up.min, 50.0);

    const LVarSpan rebased = gateway.ConsumeLVarSpan(kEng3N1);
    QCOMPARE(rebased.min, 50.0);
    QCOMPARE(rebased.max, 50.0);

    gateway.GetLVar(kEng1N1, 0.0);
    DeliverDouble(gateway, kFirstDefineId + 1, 1.0);
    DeliverDouble(gateway, kFirstDefineId + 1, 0.0);

    const LVarSpan down = gateway.ConsumeLVarSpan(kEng1N1);
    QCOMPARE(down.min, 0.0);
    QCOMPARE(down.max, 1.0);
}

void VariableGatewayTest::everyAskerSeesTheSameChangeInOneTick()
{
    SimConnectVariableGateway gateway;
    gateway.GetLVar(kEng3N1, 0.0);

    DeliverDouble(gateway, kFirstDefineId, 10.0);
    gateway.MarkTick();

    DeliverDouble(gateway, kFirstDefineId, 20.0);
    gateway.MarkTick();

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));
    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));
    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));

    gateway.MarkTick();

    QVERIFY(!gateway.HasLVarChangedThisTick(kEng3N1));
    QVERIFY(!gateway.HasLVarChangedThisTick(kEng3N1));
}

void VariableGatewayTest::theSpanPairAnswersTheSecondAskerDifferently()
{
    SimConnectVariableGateway gateway;
    gateway.GetLVar(kEng3N1, 0.0);

    DeliverDouble(gateway, kFirstDefineId, 10.0);
    gateway.MarkTick();

    DeliverDouble(gateway, kFirstDefineId, 20.0);
    gateway.MarkTick();

    const LVarSpan first = gateway.ConsumeLVarSpan(kEng3N1);
    const LVarSpan second = gateway.ConsumeLVarSpan(kEng3N1);

    QCOMPARE(first.max - first.min, 10.0);
    QCOMPARE(second.max - second.min, 0.0);

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));
}

void VariableGatewayTest::aVariableWithoutABaselineCountsAsChanged()
{
    SimConnectVariableGateway gateway;

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));

    DeliverDouble(gateway, kFirstDefineId, 10.0);
    gateway.MarkTick();

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));

    gateway.MarkTick();

    QVERIFY(!gateway.HasLVarChangedThisTick(kEng3N1));

    DeliverDouble(gateway, kFirstDefineId, 11.0);
    gateway.MarkTick();

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));
}

void VariableGatewayTest::detachForgetsTheTickBaseline()
{
    SimConnectVariableGateway gateway;
    gateway.GetLVar(kEng3N1, 0.0);

    DeliverDouble(gateway, kFirstDefineId, 10.0);
    gateway.MarkTick();
    gateway.MarkTick();

    QVERIFY(!gateway.HasLVarChangedThisTick(kEng3N1));

    gateway.Detach();
    gateway.MarkTick();

    QVERIFY(gateway.HasLVarChangedThisTick(kEng3N1));
}

void VariableGatewayTest::fastRefreshSharesSlotWithGetLVar()
{
    SimConnectVariableGateway gateway;

    gateway.SetFastRefresh(kEng3N1);
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);

    DeliverDouble(gateway, kFirstDefineId, 100.0);

    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 100.0);
}

void VariableGatewayTest::fastRefreshPromotesAlreadyReadSlotToFrameRate()
{
    FakeSimConnectApi::Reset();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    gateway.GetLVar(kEng3N1, 0.0);

    QCOMPARE(FakeSimConnectApi::dataRequests.size(), std::size_t{1});
    QCOMPARE(FakeSimConnectApi::dataRequests.back().period, SIMCONNECT_PERIOD_SECOND);

    gateway.SetFastRefresh(kEng3N1);

    QCOMPARE(FakeSimConnectApi::dataRequests.size(), std::size_t{2});
    QCOMPARE(FakeSimConnectApi::dataRequests.back().defineId, kFirstDefineId);
    QCOMPARE(FakeSimConnectApi::dataRequests.back().period, SIMCONNECT_PERIOD_SIM_FRAME);
    QCOMPARE(FakeSimConnectApi::dataRequests.back().interval, DWORD{1});
}

void VariableGatewayTest::fastRefreshKeepsFrameRateWhenAskedTwice()
{
    FakeSimConnectApi::Reset();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    gateway.SetFastRefresh(kEng3N1);
    const std::size_t afterFirst = FakeSimConnectApi::dataRequests.size();

    gateway.SetFastRefresh(kEng3N1);

    QCOMPARE(FakeSimConnectApi::dataRequests.size(), afterFirst);
    QCOMPARE(FakeSimConnectApi::dataRequests.back().period, SIMCONNECT_PERIOD_SIM_FRAME);
}

void VariableGatewayTest::forgettingTheTextSlotsDropsThePreviousFlightStrings()
{
    SimConnectVariableGateway gateway;
    char title[kString256] = {};
    char atcModel[kString256] = {};

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QVERIFY(!gateway.FetchAtcModel(atcModel, sizeof atcModel));

    DeliverText(gateway, kFirstDefineId, kSuper27FreighterTitle);
    DeliverText(gateway, kFirstDefineId + 1, kSuper27AtcModel);

    QVERIFY(gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(std::string(title), std::string(kSuper27FreighterTitle));
    QVERIFY(gateway.FetchAtcModel(atcModel, sizeof atcModel));
    QCOMPARE(std::string(atcModel), std::string(kSuper27AtcModel));

    gateway.ForgetTextSlots();

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(std::string(title), std::string());
    QVERIFY(!gateway.FetchAtcModel(atcModel, sizeof atcModel));
    QCOMPARE(std::string(atcModel), std::string());
}

void VariableGatewayTest::forgettingTheTextSlotsLeavesTheNumbersAlone()
{
    SimConnectVariableGateway gateway;
    char title[kString256] = {};

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);

    DeliverText(gateway, kFirstDefineId, kSuper27FreighterTitle);
    DeliverDouble(gateway, kFirstDefineId + 1, 84.5);

    gateway.ForgetTextSlots();

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QVERIFY(gateway.HasReceivedLVar(kEng3N1));
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 84.5);
}

void VariableGatewayTest::forgettingTheTextSlotsAsksTheSimForTheStringsAgain()
{
    FakeSimConnectApi::Reset();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    char title[kString256] = {};

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);

    const std::size_t afterTheFirstFlight = FakeSimConnectApi::dataRequests.size();

    gateway.ForgetTextSlots();

    QCOMPARE(FakeSimConnectApi::dataRequests.size(), afterTheFirstFlight + 1);
    QCOMPARE(FakeSimConnectApi::dataRequests.back().defineId, kFirstDefineId);
}

void VariableGatewayTest::forgettingTheTextSlotsRebuildsTheStringDefinitionInsteadOfAppendingToIt()
{
    FakeSimConnectApi::Reset();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    char title[kString256] = {};

    QVERIFY(!gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(FakeSimConnectApi::DatumsIn(kFirstDefineId), std::size_t{1});

    gateway.ForgetTextSlots();
    gateway.ForgetTextSlots();

    QCOMPARE(FakeSimConnectApi::DatumsIn(kFirstDefineId), std::size_t{1});
}

void VariableGatewayTest::everyWriteToTheSimLandsInTheWritesLog()
{
    FakeSimConnectApi::Reset();
    const qsizetype before = ProbeLines(kWritesLog).size();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    gateway.SetLVar(kGpuAvail, 1.0);
    gateway.SetAVar(kPayloadStation, kPoundsUnit, 1234.5);

    QCOMPARE(ProbeLines(kWritesLog).mid(before),
             (QStringList{
                 QStringLiteral("set L:FSS_B727_GPU_AVAIL=1 unit=Number n=1"),
                 QStringLiteral("set PAYLOAD STATION WEIGHT:1=1234.5 unit=pounds n=1")
             }));
}

void VariableGatewayTest::aRepeatedWriteIsCountedAndLoggedOnceUntilTheValueChanges()
{
    FakeSimConnectApi::Reset();
    const qsizetype before = ProbeLines(kWritesLog).size();
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    gateway.SetLVar(kGpuAvail, 1.0);
    gateway.SetLVar(kGpuAvail, 1.0);
    gateway.SetLVar(kGpuAvail, 1.0);
    gateway.SetLVar(kGpuAvail, 0.0);

    QCOMPARE(FakeSimConnectApi::writtenSimObjectData.size(), std::size_t{4});
    QCOMPARE(ProbeLines(kWritesLog).mid(before),
             (QStringList{
                 QStringLiteral("set L:FSS_B727_GPU_AVAIL=1 unit=Number n=1"),
                 QStringLiteral("set L:FSS_B727_GPU_AVAIL=0 unit=Number n=4")
             }));
}

void VariableGatewayTest::aWriteThatNeverLeftIsNotLogged()
{
    FakeSimConnectApi::Reset();
    const qsizetype before = ProbeLines(kWritesLog).size();
    SimConnectVariableGateway gateway;

    gateway.SetLVar(kGpuAvail, 1.0);

    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());
    QCOMPARE(ProbeLines(kWritesLog).size(), before);
}

QTEST_APPLESS_MAIN(VariableGatewayTest)

#include "tst_variable_gateway.moc"

void VariableGatewayTest::aRejectedRequestKeepsTheDefaultUntilItIsAskedAgainAndAnswered()
{
    Rejections rig;
    QCOMPARE(rig.gateway.GetLVar(kEng3N1, -1.0), -1.0);
    QCOMPARE(rig.Requests(), std::size_t{1});

    rig.RejectLastRequest();

    QCOMPARE(rig.gateway.GetLVar(kEng3N1, -1.0), -1.0);
    QVERIFY(!rig.gateway.HasReceivedLVar(kEng3N1));

    rig.Wait(kFirstRetryMs);

    QCOMPARE(rig.Requests(), std::size_t{2});
    QCOMPARE(FakeSimConnectApi::dataRequests.back().period, SIMCONNECT_PERIOD_SECOND);

    DeliverDouble(rig.gateway, kFirstDefineId, 84.5);

    QCOMPARE(rig.gateway.GetLVar(kEng3N1, -1.0), 84.5);
    QVERIFY(rig.gateway.HasReceivedLVar(kEng3N1));
}

void VariableGatewayTest::aRejectedRequestIsNotAskedAgainBeforeTheBackoffPasses()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    rig.RejectLastRequest();

    rig.Wait(0);
    rig.Wait(kFirstRetryMs - 1);

    QCOMPARE(rig.Requests(), std::size_t{1});

    rig.Wait(1);

    QCOMPARE(rig.Requests(), std::size_t{2});

    rig.Wait(kLongestWaitMs);

    QCOMPARE(rig.Requests(), std::size_t{2});
}

void VariableGatewayTest::theBackoffDoublesUpToACeiling()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);

    const long long expectedWaits[] = {1000, 2000, 4000, 8000, 16000, 16000, 16000};
    std::size_t requests = 1;
    for (const long long wait : expectedWaits)
    {
        rig.RejectLastRequest();

        rig.Wait(wait - 1);
        QCOMPARE(rig.Requests(), requests);

        rig.Wait(1);
        QCOMPARE(rig.Requests(), ++requests);
    }
}

void VariableGatewayTest::aRejectedAircraftTitleIsAskedAgain()
{
    Rejections rig;
    char title[kString256] = {};
    QVERIFY(!rig.gateway.FetchAircraftName(title, sizeof title));
    rig.RejectLastRequest();

    rig.Wait(kFirstRetryMs);

    QCOMPARE(rig.Requests(), std::size_t{2});

    DeliverText(rig.gateway, kFirstDefineId, kSuper27FreighterTitle);

    QVERIFY(rig.gateway.FetchAircraftName(title, sizeof title));
    QCOMPARE(std::string(title), std::string(kSuper27FreighterTitle));
}

void VariableGatewayTest::aRetryRebuildsTheDefinitionInsteadOfAppendingToIt()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    QCOMPARE(FakeSimConnectApi::DatumsIn(kFirstDefineId), std::size_t{1});

    for (int attempt = 0; attempt < 3; ++attempt)
    {
        rig.RejectLastRequest();
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(FakeSimConnectApi::DatumsIn(kFirstDefineId), std::size_t{1});
    QCOMPARE(FakeSimConnectApi::clearedDefinitions.size(), std::size_t{3});
}

void VariableGatewayTest::aRejectedDefinitionOfAnLVarIsAskedAgainAndTheFollowUpRejectionIsAbsorbed()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    const DWORD definitionSendId = FakeSimConnectApi::LastDefinitionSendId(kFirstDefineId);
    const DWORD requestSendId = FakeSimConnectApi::dataRequests.back().sendId;
    QVERIFY(definitionSendId != requestSendId);

    rig.Reject(kNameUnrecognized, definitionSendId);
    rig.Reject(kUnrecognizedId, requestSendId);

    rig.Wait(kFirstRetryMs);

    QCOMPARE(rig.Requests(), std::size_t{2});

    rig.Wait(kLongestWaitMs);

    QCOMPARE(rig.Requests(), std::size_t{2});

    DeliverDouble(rig.gateway, kFirstDefineId, 3.0);

    QCOMPARE(rig.gateway.GetLVar(kEng3N1, 0.0), 3.0);
}

void VariableGatewayTest::aRejectedDefinitionOfAnAVarIsNotAskedAgain()
{
    Rejections rig;
    QCOMPARE(rig.gateway.GetAVar(kSimFuelTotalKg, kUnitTypo, 0.0), 0.0);
    const DWORD definitionSendId = FakeSimConnectApi::LastDefinitionSendId(kFirstDefineId);
    const DWORD requestSendId = FakeSimConnectApi::dataRequests.back().sendId;

    rig.Reject(kNameUnrecognized, definitionSendId);
    rig.Reject(kUnrecognizedId, requestSendId);

    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{1});
    QCOMPARE(FakeSimConnectApi::clearedDefinitions.size(), std::size_t{0});
    QCOMPARE(rig.gateway.GetAVar(kSimFuelTotalKg, kUnitTypo, -1.0), -1.0);
}

void VariableGatewayTest::anExceptionThatIsNotATimingProblemIsNotAskedAgain_data()
{
    QTest::addColumn<DWORD>("exception");

    QTest::newRow("error") << DWORD{1};
    QTest::newRow("size mismatch") << DWORD{2};
    QTest::newRow("unopened") << DWORD{4};
    QTest::newRow("too many requests") << DWORD{12};
    QTest::newRow("illegal operation") << kIllegalOperation;
    QTest::newRow("definition error") << DWORD{28};
}

void VariableGatewayTest::anExceptionThatIsNotATimingProblemIsNotAskedAgain()
{
    QFETCH(DWORD, exception);

    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    rig.RejectLastRequest(exception);

    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{1});
}

void VariableGatewayTest::theRetriesStopAtTheBudget()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);

    for (unsigned retry = 1; retry <= kRetryBudget; ++retry)
    {
        rig.RejectLastRequest();
        rig.Wait(kLongestWaitMs);

        QCOMPARE(rig.Requests(), std::size_t{retry + 1});
    }

    rig.RejectLastRequest();
    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{kRetryBudget + 1});
    QCOMPARE(rig.gateway.GetLVar(kEng3N1, -1.0), -1.0);
}

void VariableGatewayTest::reconnectingGivesTheBudgetBack()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);

    for (unsigned retry = 0; retry <= kRetryBudget; ++retry)
    {
        rig.RejectLastRequest();
        rig.Wait(kLongestWaitMs);
    }

    const std::size_t exhausted = rig.Requests();

    rig.gateway.Detach();
    rig.gateway.Attach(reinterpret_cast<HANDLE>(0x5150));

    QCOMPARE(rig.Requests(), exhausted + 1);

    rig.RejectLastRequest();
    rig.Wait(kFirstRetryMs);

    QCOMPARE(rig.Requests(), exhausted + 2);
}

void VariableGatewayTest::anExceptionForAnotherCallIsIgnored()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    const DWORD requestSendId = FakeSimConnectApi::dataRequests.back().sendId;

    rig.Reject(kUnrecognizedId, 0);
    rig.Reject(kUnrecognizedId, requestSendId + 1);
    rig.Reject(kUnrecognizedId, requestSendId + 100);

    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{1});
}

void VariableGatewayTest::anExceptionAfterTheDataArrivedIsIgnored()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    DeliverDouble(rig.gateway, kFirstDefineId, 12.0);

    rig.RejectLastRequest();

    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{1});
    QCOMPARE(rig.gateway.GetLVar(kEng3N1, 0.0), 12.0);
}

void VariableGatewayTest::detachingCancelsThePendingRetry()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    rig.RejectLastRequest();

    rig.gateway.Detach();
    rig.Wait(kLongestWaitMs);

    QCOMPARE(rig.Requests(), std::size_t{1});
}

void VariableGatewayTest::aRetryThatCouldNotBeSentIsScheduledAgain()
{
    Rejections rig;
    rig.gateway.GetLVar(kEng3N1, 0.0);
    rig.RejectLastRequest();

    FakeSimConnectApi::definitionsSucceed = false;
    rig.Wait(kFirstRetryMs);

    QCOMPARE(rig.Requests(), std::size_t{1});

    FakeSimConnectApi::definitionsSucceed = true;
    rig.Wait(2 * kFirstRetryMs - 1);

    QCOMPARE(rig.Requests(), std::size_t{1});

    rig.Wait(1);

    QCOMPARE(rig.Requests(), std::size_t{2});
}

void VariableGatewayTest::anExceptionWithoutASendIdIsNeverAttributedToAVariable()
{
    Rejections rig;
    FakeSimConnectApi::lastSentPacketId = (std::numeric_limits<DWORD>::max)();
    rig.gateway.GetLVar(kEng3N1, 0.0);
    QCOMPARE(FakeSimConnectApi::LastDefinitionSendId(kFirstDefineId), DWORD{0});

    rig.Reject(kUnrecognizedId, 0);

    for (int wait = 0; wait < 40; ++wait)
    {
        rig.Wait(kLongestWaitMs);
    }

    QCOMPARE(rig.Requests(), std::size_t{1});
}
