#include <QtTest/QTest>
#include <QtCore/QTemporaryDir>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/infrastructure/aircraft/pmdg/Pmdg737.h"
#include "../src/infrastructure/aircraft/pmdg/Pmdg777.h"
#include "../src/infrastructure/pmdg/PmdgOptions.h"

namespace
{
    std::string ReadAll(const std::filesystem::path& path)
    {
        const std::ifstream input(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << input.rdbuf();

        return buffer.str();
    }

    void WriteAll(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << content;
    }
}

class PmdgOptionsTest final : public QObject
{
    Q_OBJECT

private slots:
    void readsDataBroadcastFlag();
    void appendsSectionWhenAbsent();
    void keepsExistingSectionsWhenAppending();
    void flipsDisabledFlagInPlace();
    void addsKeyToExistingSection();
    void leavesEnabledTextUnchanged();
    void keepsCarriageReturnsOfTheFile();
    void enablesBroadcastOnDisk();
    void reportsAbsentFileOnDisk();
    void mapsEveryPmdgNameToItsPackage();
    void mapsTheSevenThirtySevenFamilyToItsOwnPackage();
    void ignoresAircraftWithoutOptionsFile();
};

void PmdgOptionsTest::readsDataBroadcastFlag()
{
    QVERIFY(PmdgOptions::HasDataBroadcast("[SDK]\r\nEnableDataBroadcast=1\r\n"));
    QVERIFY(PmdgOptions::HasDataBroadcast("[Misc]\nFoo=2\n[SDK]\nEnableDataBroadcast = 1\n"));
    QVERIFY(!PmdgOptions::HasDataBroadcast("[SDK]\nEnableDataBroadcast=0\n"));
    QVERIFY(!PmdgOptions::HasDataBroadcast("[Misc]\nEnableDataBroadcast=1\n"));
    QVERIFY(!PmdgOptions::HasDataBroadcast(""));
}

void PmdgOptionsTest::appendsSectionWhenAbsent()
{
    const std::string fixed = PmdgOptions::WithDataBroadcast("");

    QVERIFY(PmdgOptions::HasDataBroadcast(fixed));
    QCOMPARE(fixed, std::string("[SDK]\nEnableDataBroadcast=1\n"));
}

void PmdgOptionsTest::keepsExistingSectionsWhenAppending()
{
    const std::string original = "[IRS.0]\nLastPosValid=0\n\n[IRS.1]\nLastPosValid=0\n";

    const std::string fixed = PmdgOptions::WithDataBroadcast(original);

    QVERIFY(PmdgOptions::HasDataBroadcast(fixed));
    QVERIFY(fixed.starts_with(original));
    QVERIFY(fixed.find("[IRS.0]") != std::string::npos);
    QVERIFY(fixed.find("[IRS.1]") != std::string::npos);
}

void PmdgOptionsTest::flipsDisabledFlagInPlace()
{
    const std::string fixed =
        PmdgOptions::WithDataBroadcast("[SDK]\nEnableDataBroadcast=0\n[Misc]\nFoo=2\n");

    QVERIFY(PmdgOptions::HasDataBroadcast(fixed));
    QVERIFY(fixed.find("EnableDataBroadcast=0") == std::string::npos);
    QVERIFY(fixed.find("[Misc]\nFoo=2") != std::string::npos);
}

void PmdgOptionsTest::addsKeyToExistingSection()
{
    const std::string fixed = PmdgOptions::WithDataBroadcast("[SDK]\nEnableCDUBroadcast.0=1\n");

    QVERIFY(PmdgOptions::HasDataBroadcast(fixed));
    QVERIFY(fixed.find("EnableCDUBroadcast.0=1") != std::string::npos);
}

void PmdgOptionsTest::leavesEnabledTextUnchanged()
{
    const std::string original = "[SDK]\nEnableDataBroadcast=1\n";

    QCOMPARE(PmdgOptions::WithDataBroadcast(original), original);
}

void PmdgOptionsTest::keepsCarriageReturnsOfTheFile()
{
    const std::string fixed = PmdgOptions::WithDataBroadcast("[IRS.0]\r\nLastPosValid=0\r\n");

    QVERIFY(PmdgOptions::HasDataBroadcast(fixed));
    QVERIFY(fixed.find("[SDK]\r\n") != std::string::npos);
    QVERIFY(fixed.find("[SDK]\n\r") == std::string::npos);
}

void PmdgOptionsTest::enablesBroadcastOnDisk()
{
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const std::filesystem::path ini =
        std::filesystem::path(dir.path().toStdWString()) / "737_Options.ini";
    WriteAll(ini, "[IRS.0]\nLastPosValid=0\n");

    QCOMPARE(PmdgOptions::ReadDataBroadcast(ini), std::optional(false));
    QVERIFY(PmdgOptions::EnableDataBroadcast(ini));
    QCOMPARE(PmdgOptions::ReadDataBroadcast(ini), std::optional(true));

    const std::string written = ReadAll(ini);
    QVERIFY(written.find("[IRS.0]") != std::string::npos);
    QVERIFY(PmdgOptions::HasDataBroadcast(written));
}

void PmdgOptionsTest::reportsAbsentFileOnDisk()
{
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const std::filesystem::path missing =
        std::filesystem::path(dir.path().toStdWString()) / "does-not-exist.ini";

    QCOMPARE(PmdgOptions::ReadDataBroadcast(missing), std::nullopt);
    QVERIFY(!PmdgOptions::EnableDataBroadcast(missing));
}

void PmdgOptionsTest::mapsEveryPmdgNameToItsPackage()
{
    const auto pathFor = [](const char* name) {
        const std::optional<std::filesystem::path> path = PmdgOptions::PathFor(name);
        return path.has_value() ? path->generic_string() : std::string();
    };

    QVERIFY(pathFor(Pmdg777::kName300Er).ends_with("pmdg-aircraft-77w/work/777_Options.ini"));
    QVERIFY(pathFor(Pmdg777::kNameFreighter).ends_with("pmdg-aircraft-77f/work/777_Options.ini"));
    QVERIFY(pathFor(Pmdg777::kName200Lr).ends_with("pmdg-aircraft-77l/work/777_Options.ini"));
    QVERIFY(pathFor(Pmdg777::kName200Er).ends_with("pmdg-aircraft-77er/work/777_Options.ini"));
}

void PmdgOptionsTest::mapsTheSevenThirtySevenFamilyToItsOwnPackage()
{
    const auto pathFor = [](const char* name) {
        const std::optional<std::filesystem::path> path = PmdgOptions::PathFor(name);
        return path.has_value() ? path->generic_string() : std::string();
    };

    for (const char* name : {Pmdg737::kNamePax800, Pmdg737::kNameBcf800,
                             Pmdg737::kNameBdsf800, Pmdg737::kNameBbj2})
    {
        QVERIFY2(pathFor(name).ends_with("pmdg-aircraft-738/work/737_Options.ini"), name);
    }
}

void PmdgOptionsTest::ignoresAircraftWithoutOptionsFile()
{
    QCOMPARE(PmdgOptions::PathFor("ToLiss A340-600"), std::nullopt);
    QCOMPARE(PmdgOptions::PathFor(""), std::nullopt);
}

QTEST_APPLESS_MAIN(PmdgOptionsTest)

#include "tst_pmdg_options.moc"
