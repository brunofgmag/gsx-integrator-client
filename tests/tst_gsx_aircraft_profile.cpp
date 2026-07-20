#include <QtTest/QTest>

#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/infrastructure/gsx/GsxAircraftProfile.h"

namespace
{
    constexpr auto kSampleCfg =
        "[aircraft]\n"
        "icaotype = A346\n"
        "refueling = 1\n"
        "battery = 1\n"
        "\n"
        "[exit1]\n"
        "refueling = 9\n"
        "pos = 1.0 2.0\n";

    std::filesystem::path WriteCfg(const QTemporaryDir& dir, const std::string& content)
    {
        const std::filesystem::path path = std::filesystem::path(dir.path().toStdString()) / "gsx.cfg";
        std::ofstream file(path, std::ios::binary);
        file << content;

        return path;
    }

    std::string ReadAll(const std::filesystem::path& path)
    {
        const std::ifstream file(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << file.rdbuf();

        return buffer.str();
    }
}

class GsxAircraftProfileTest final : public QObject
{
    Q_OBJECT

private slots:
    static void readsRefuelingFromAircraftSection();
    static void readIgnoresOtherSections();
    static void readMissingKeyReturnsNothing();
    static void readMissingFileReturnsNothing();
    static void writeReplacesOnlyTheRefuelingValue();
    static void writePreservesCrlfEndings();
    static void writeInsertsKeyWhenMissing();
    static void writeFailsWithoutAircraftSection();
    static void writeFailsWhenFileMissing();
    static void profileRootsKnownForTolissA340();
    static void profileRootsKnownForTfdiMd11();
    static void profileRootsKnownForFenixA32x();
    static void profileRootsEmptyForUnknownAircraft();
    static void flagsMissingProfileOnlyForTolissA340();
    static void findCfgsScansRootsRecursively();
    static void findCfgsIgnoresMissingRoots();
};

void GsxAircraftProfileTest::readsRefuelingFromAircraftSection()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, kSampleCfg);

    const auto refueling = GsxAircraftProfile::ReadRefueling(path);

    QVERIFY(refueling.has_value());
    QCOMPARE(*refueling, 1);
}

void GsxAircraftProfileTest::readIgnoresOtherSections()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, "[exit1]\nrefueling = 9\n");

    QVERIFY(!GsxAircraftProfile::ReadRefueling(path).has_value());
}

void GsxAircraftProfileTest::readMissingKeyReturnsNothing()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, "[aircraft]\nicaotype = A346\n");

    QVERIFY(!GsxAircraftProfile::ReadRefueling(path).has_value());
}

void GsxAircraftProfileTest::readMissingFileReturnsNothing()
{
    const QTemporaryDir dir;
    const std::filesystem::path path =
        std::filesystem::path(dir.path().toStdString()) / "missing.cfg";

    QVERIFY(!GsxAircraftProfile::ReadRefueling(path).has_value());
}

void GsxAircraftProfileTest::writeReplacesOnlyTheRefuelingValue()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, kSampleCfg);

    QVERIFY(GsxAircraftProfile::WriteRefueling(path, 0));

    const std::string expected =
        "[aircraft]\n"
        "icaotype = A346\n"
        "refueling = 0\n"
        "battery = 1\n"
        "\n"
        "[exit1]\n"
        "refueling = 9\n"
        "pos = 1.0 2.0\n";

    QCOMPARE(ReadAll(path), expected);
    QCOMPARE(*GsxAircraftProfile::ReadRefueling(path), 0);
}

void GsxAircraftProfileTest::writePreservesCrlfEndings()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, "[aircraft]\r\nrefueling = 1\r\nbattery = 1\r\n");

    QVERIFY(GsxAircraftProfile::WriteRefueling(path, 0));

    QCOMPARE(ReadAll(path), std::string("[aircraft]\r\nrefueling = 0\r\nbattery = 1\r\n"));
}

void GsxAircraftProfileTest::writeInsertsKeyWhenMissing()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, "[aircraft]\nicaotype = A346\n");

    QVERIFY(GsxAircraftProfile::WriteRefueling(path, 0));

    QCOMPARE(ReadAll(path), std::string("[aircraft]\nrefueling = 0\nicaotype = A346\n"));
}

void GsxAircraftProfileTest::writeFailsWithoutAircraftSection()
{
    const QTemporaryDir dir;
    const auto path = WriteCfg(dir, "[exit1]\npos = 1.0\n");

    QVERIFY(!GsxAircraftProfile::WriteRefueling(path, 0));
    QCOMPARE(ReadAll(path), std::string("[exit1]\npos = 1.0\n"));
}

void GsxAircraftProfileTest::writeFailsWhenFileMissing()
{
    const QTemporaryDir dir;
    const std::filesystem::path path =
        std::filesystem::path(dir.path().toStdString()) / "missing.cfg";

    QVERIFY(!GsxAircraftProfile::WriteRefueling(path, 0));
}

void GsxAircraftProfileTest::profileRootsKnownForTolissA340()
{
    const auto roots = GsxAircraftProfile::ProfileRootsFor("ToLiss A340-600");

    QCOMPARE(roots.size(), static_cast<std::size_t>(2));

    const std::string airplanes = (std::filesystem::path("Virtuali") / "Airplanes").string();

    QVERIFY(roots[0].string().find(airplanes) != std::string::npos);
    QCOMPARE(roots[0].filename().string(), std::string("airbus-a346-pro"));
    QCOMPARE(roots[1].filename().string(), std::string("aerosoft-a340-600-pro"));
}

void GsxAircraftProfileTest::profileRootsKnownForTfdiMd11()
{
    const auto roots = GsxAircraftProfile::ProfileRootsFor("TFDi MD-11");

    QCOMPARE(roots.size(), static_cast<std::size_t>(1));

    const std::string airplanes = (std::filesystem::path("Virtuali") / "Airplanes").string();

    QVERIFY(roots[0].string().find(airplanes) != std::string::npos);
    QCOMPARE(roots[0].filename().string(), std::string("tfdi_design_md-11"));
}

void GsxAircraftProfileTest::profileRootsKnownForFenixA32x()
{
    for (const auto* aircraftName : {"Fenix A319", "Fenix A320", "Fenix A321"})
    {
        const auto roots = GsxAircraftProfile::ProfileRootsFor(aircraftName);

        QCOMPARE(roots.size(), static_cast<std::size_t>(1));

        const std::string airplanes = (std::filesystem::path("Virtuali") / "Airplanes").string();

        QVERIFY(roots[0].string().find(airplanes) != std::string::npos);
        QCOMPARE(roots[0].filename().string(), std::string("FNX_32X"));
    }
}

void GsxAircraftProfileTest::profileRootsEmptyForUnknownAircraft()
{
    QVERIFY(GsxAircraftProfile::ProfileRootsFor("Unknown Aircraft").empty());
}

void GsxAircraftProfileTest::flagsMissingProfileOnlyForTolissA340()
{
    QVERIFY(GsxAircraftProfile::FlagsMissingProfile("ToLiss A340-600"));
    QVERIFY(!GsxAircraftProfile::FlagsMissingProfile("TFDi MD-11"));
    QVERIFY(!GsxAircraftProfile::FlagsMissingProfile("Fenix A320"));
    QVERIFY(!GsxAircraftProfile::FlagsMissingProfile("Unknown Aircraft"));
}

void GsxAircraftProfileTest::findCfgsScansRootsRecursively()
{
    const QTemporaryDir dir;
    const std::filesystem::path base(dir.path().toStdString());
    const std::filesystem::path rootA = base / "airbus-a346-pro";
    const std::filesystem::path rootB = base / "aerosoft-a340-600-pro";
    std::filesystem::create_directories(rootA / "600");
    std::filesystem::create_directories(rootB / "some" / "deep");

    std::ofstream(rootA / "600" / "gsx.cfg") << "[aircraft]\n";
    std::ofstream(rootA / "gsx.cfg") << "[aircraft]\n";
    std::ofstream(rootB / "some" / "deep" / "GSX.CFG") << "[aircraft]\n";
    std::ofstream(rootA / "600" / "gsx_handler.py") << "pass\n";

    const auto cfgs = GsxAircraftProfile::FindCfgs({rootA, rootB});

    QCOMPARE(cfgs.size(), static_cast<std::size_t>(3));
}

void GsxAircraftProfileTest::findCfgsIgnoresMissingRoots()
{
    const QTemporaryDir dir;
    const std::filesystem::path missing = std::filesystem::path(dir.path().toStdString()) / "does-not-exist";

    QVERIFY(GsxAircraftProfile::FindCfgs({missing}).empty());
}

QTEST_APPLESS_MAIN(GsxAircraftProfileTest)

#include "tst_gsx_aircraft_profile.moc"
