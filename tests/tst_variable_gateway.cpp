#include <QtTest/QTest>

#include <cstring>
#include <vector>
#include <windows.h>
#include <SimConnect.h>
#include "../src/infrastructure/simconnect/SimConnectVariableGateway.h"

namespace
{
    constexpr auto kEng1N1 = "md11_eng1_n1";
    constexpr auto kEng3N1 = "md11_eng3_n1";
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kKgUnit = "kg";
    constexpr DWORD kFirstDefineId = 1;

    void DeliverDouble(SimConnectVariableGateway& gateway, const DWORD requestId, const double value)
    {
        std::vector<BYTE> buffer(sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) + sizeof(double), 0);
        const auto data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(buffer.data());
        data->dwRequestID = requestId;
        std::memcpy(&data->dwData, &value, sizeof(double));
        gateway.HandleSimObjectData(data);
    }
}

class VariableGatewayTest final : public QObject
{
    Q_OBJECT

private slots:
    static void lvarReturnsDefaultUntilDataArrives();
    static void lvarReturnsRealValueAfterDataArrives();
    static void lvarHonorsCustomDefault();
    static void dataForOneSlotDoesNotLeakToAnother();
    static void detachResetsReceivedState();
    static void avarReportsReceivedOnlyAfterDataArrives();
    static void lvarReportsReceivedOnlyAfterDataArrives();
    static void fastRefreshSharesSlotWithGetLVar();
    static void consumeLVarSpanCatchesTransient();
};

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

void VariableGatewayTest::fastRefreshSharesSlotWithGetLVar()
{
    SimConnectVariableGateway gateway;

    gateway.SetFastRefresh(kEng3N1);
    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 0.0);

    DeliverDouble(gateway, kFirstDefineId, 100.0);

    QCOMPARE(gateway.GetLVar(kEng3N1, 0.0), 100.0);
}

QTEST_APPLESS_MAIN(VariableGatewayTest)

#include "tst_variable_gateway.moc"
