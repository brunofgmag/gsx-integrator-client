#include <QtTest/QTest>

#include <cstddef>
#include <string>
#include "../src/infrastructure/pmdg/Pmdg737SdkData.h"

class Pmdg737SdkDataTest final : public QObject
{
    Q_OBJECT

private slots:
    void marshalsToTheSizeThePmdgWasmPublishes();
    void keepsTheOffsetsTheAdapterReads();
    void keepsTheAreaIdentifiers();
};

void Pmdg737SdkDataTest::marshalsToTheSizeThePmdgWasmPublishes()
{
    QCOMPARE(sizeof(PMDG_NG3_Data), static_cast<std::size_t>(916));
}

void Pmdg737SdkDataTest::keepsTheOffsetsTheAdapterReads()
{
    QCOMPARE(offsetof(PMDG_NG3_Data, IRS_aligned), static_cast<std::size_t>(13));
    QCOMPARE(offsetof(PMDG_NG3_Data, FUEL_QtyCenter), static_cast<std::size_t>(116));
    QCOMPARE(offsetof(PMDG_NG3_Data, FUEL_QtyLeft), static_cast<std::size_t>(120));
    QCOMPARE(offsetof(PMDG_NG3_Data, FUEL_QtyRight), static_cast<std::size_t>(124));
    QCOMPARE(offsetof(PMDG_NG3_Data, ELEC_annunGRD_POWER_AVAILABLE), static_cast<std::size_t>(142));
    QCOMPARE(offsetof(PMDG_NG3_Data, ELEC_GrdPwrSw), static_cast<std::size_t>(143));
    QCOMPARE(offsetof(PMDG_NG3_Data, ELEC_BusPowered), static_cast<std::size_t>(182));
    QCOMPARE(offsetof(PMDG_NG3_Data, DOOR_annunFWD_ENTRY), static_cast<std::size_t>(344));
    QCOMPARE(offsetof(PMDG_NG3_Data, DOOR_annunAIRSTAIR), static_cast<std::size_t>(346));
    QCOMPARE(offsetof(PMDG_NG3_Data, DOOR_annunFWD_CARGO), static_cast<std::size_t>(349));
    QCOMPARE(offsetof(PMDG_NG3_Data, DOOR_annunAFT_CARGO), static_cast<std::size_t>(353));
    QCOMPARE(offsetof(PMDG_NG3_Data, DOOR_annunAFT_ENTRY), static_cast<std::size_t>(354));
    QCOMPARE(offsetof(PMDG_NG3_Data, APU_Selector), static_cast<std::size_t>(379));
    QCOMPARE(offsetof(PMDG_NG3_Data, LTS_AntiCollisionSw), static_cast<std::size_t>(385));
    QCOMPARE(offsetof(PMDG_NG3_Data, COMM_Attend_PressCount), static_cast<std::size_t>(555));
    QCOMPARE(offsetof(PMDG_NG3_Data, COMM_GrdCall_PressCount), static_cast<std::size_t>(556));
    QCOMPARE(offsetof(PMDG_NG3_Data, PED_annunParkingBrake), static_cast<std::size_t>(574));
    QCOMPARE(offsetof(PMDG_NG3_Data, AircraftModel), static_cast<std::size_t>(654));
    QCOMPARE(offsetof(PMDG_NG3_Data, WeightInKg), static_cast<std::size_t>(656));
    QCOMPARE(offsetof(PMDG_NG3_Data, GroundConnAvailable), static_cast<std::size_t>(658));
}

void Pmdg737SdkDataTest::keepsTheAreaIdentifiers()
{
    QCOMPARE(std::string(PMDG_NG3_DATA_NAME), std::string("PMDG_NG3_Data"));
    QCOMPARE(PMDG_NG3_DATA_ID, 0x4E473331);
    QCOMPARE(PMDG_NG3_DATA_DEFINITION, 0x4E473332);
}

QTEST_APPLESS_MAIN(Pmdg737SdkDataTest)

#include "tst_pmdg737_sdk_data.moc"
