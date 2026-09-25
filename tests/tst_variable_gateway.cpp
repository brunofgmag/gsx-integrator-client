#include <QtTest/QTest>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>
#include <SimConnect.h>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "ProbeLines.h"
#include "doubles/FakeSimConnectApi.h"
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
#ifndef NDEBUG
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
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void VariableGatewayTest::aRepeatedWriteIsCountedAndLoggedOnceUntilTheValueChanges()
{
#ifndef NDEBUG
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
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void VariableGatewayTest::aWriteThatNeverLeftIsNotLogged()
{
#ifndef NDEBUG
    FakeSimConnectApi::Reset();
    const qsizetype before = ProbeLines(kWritesLog).size();
    SimConnectVariableGateway gateway;

    gateway.SetLVar(kGpuAvail, 1.0);

    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());
    QCOMPARE(ProbeLines(kWritesLog).size(), before);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_APPLESS_MAIN(VariableGatewayTest)

#include "tst_variable_gateway.moc"
