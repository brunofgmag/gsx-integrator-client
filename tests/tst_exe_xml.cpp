#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/simulator/ExeXml.h"

namespace
{
    constexpr auto kClientExe = "gsx-integrator-client.exe";

    QString ReadAll(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        return QString::fromUtf8(file.readAll());
    }

    void WriteAll(const QString& path, const QString& content)
    {
        QFile file(path);

        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));

        file.write(content.toUtf8());
    }

    QString WithAddons(const QString& addons)
    {
        return QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   "<SimBase.Document Type=\"Launch\" version=\"1,0\">\n"
                   "  <Descr>Launch</Descr>\n")
               + addons + QStringLiteral("</SimBase.Document>\n");
    }

    QString OtherToolAddon()
    {
        return QStringLiteral(
            "  <Launch.Addon>\n"
            "    <Name>Other Tool</Name>\n"
            "    <Path>C:\\Other\\other-tool.exe</Path>\n"
            "  </Launch.Addon>\n");
    }

    QString ClientAddon(const QString& disabled)
    {
        return QStringLiteral(
                   "  <Launch.Addon>\n"
                   "    <Disabled>%1</Disabled>\n"
                   "    <Name>GSX Integrator</Name>\n"
                   "    <Path>C:\\Apps\\GSX-Integrator-Client.exe</Path>\n"
                   "  </Launch.Addon>\n")
            .arg(disabled);
    }
}

class ExeXmlTest final : public QObject
{
    Q_OBJECT

private slots:
    static void addUpdateCreatesFileWithEntry();
    static void addUpdateCreatesTheInstallerSkeleton();
    static void addUpdateUpdatesInsteadOfDuplicating();
    static void addUpdatePreservesOtherAddons();
    static void addUpdateReEnablesADisabledEntry();
    static void addUpdateWithoutFolderCreatesNothing();
    static void removeRemovesOnlyOurEntry();
    static void removeMissingFileIsSuccess();
    static void enabledEntryIsFoundCaseInsensitively();
    static void disabledOrAbsentEntryIsNotEnabled();
    static void candidatesAreTheFoldersThatExist();
};

void ExeXmlTest::addUpdateCreatesFileWithEntry()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\Apps\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    const QString content = ReadAll(path);

    QVERIFY(content.contains(QStringLiteral("SimBase.Document")));
    QVERIFY(content.contains(QStringLiteral("Launch.Addon")));
    QVERIFY(content.contains(QStringLiteral("gsx-integrator-client.exe")));
    QVERIFY(content.contains(QStringLiteral("GSX Integrator")));
    QVERIFY(content.contains(QStringLiteral("<CommandLine>--tray</CommandLine>")));
}

void ExeXmlTest::addUpdateCreatesTheInstallerSkeleton()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\Apps\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    const QString content = ReadAll(path);

    QVERIFY(content.contains(QStringLiteral("Type=\"Launch\"")));
    QVERIFY(content.contains(QStringLiteral("version=\"1,0\"")));
    QVERIFY(content.contains(QStringLiteral("<Descr>Launch</Descr>")));
    QVERIFY(content.contains(QStringLiteral("<Filename>EXE.xml</Filename>")));
    QVERIFY(content.contains(QStringLiteral("<Launch.ManualLoad>False</Launch.ManualLoad>")));
    QVERIFY(content.contains(QStringLiteral("<ManualLoad>False</ManualLoad>")));
    QCOMPARE(static_cast<int>(content.count(QStringLiteral("<Disabled>False</Disabled>"))), 2);
}

void ExeXmlTest::addUpdateUpdatesInsteadOfDuplicating()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\Old\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));
    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\New\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    const QString content = ReadAll(path);

    QCOMPARE(static_cast<int>(content.count(QStringLiteral("<Launch.Addon>"))), 1);
    QVERIFY(content.contains(QStringLiteral("C:\\New\\gsx-integrator-client.exe")));
    QVERIFY(!content.contains(QStringLiteral("C:\\Old\\gsx-integrator-client.exe")));
    QCOMPARE(static_cast<int>(content.count(QStringLiteral("<CommandLine>--tray</CommandLine>"))), 1);
}

void ExeXmlTest::addUpdatePreservesOtherAddons()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    WriteAll(path, WithAddons(OtherToolAddon()));

    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\Apps\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    const QString content = ReadAll(path);

    QVERIFY(content.contains(QStringLiteral("other-tool.exe")));
    QVERIFY(content.contains(QStringLiteral("gsx-integrator-client.exe")));
    QCOMPARE(static_cast<int>(content.count(QStringLiteral("<Launch.Addon>"))), 2);
}

void ExeXmlTest::addUpdateReEnablesADisabledEntry()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    WriteAll(path, WithAddons(ClientAddon(QStringLiteral("True"))));

    QVERIFY(ExeXmlAddUpdate(path, QStringLiteral("C:\\Apps\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    QVERIFY(ExeXmlHasEnabledEntry(path, QLatin1String(kClientExe)));
    QCOMPARE(static_cast<int>(ReadAll(path).count(QStringLiteral("<Launch.Addon>"))), 1);
}

void ExeXmlTest::addUpdateWithoutFolderCreatesNothing()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("missing/EXE.xml"));

    QVERIFY(!ExeXmlAddUpdate(path, QStringLiteral("C:\\Apps\\gsx-integrator-client.exe"),
        QStringLiteral("GSX Integrator")));

    QVERIFY(!QFile::exists(path));
    QVERIFY(!QDir(dir.filePath(QStringLiteral("missing"))).exists());
}

void ExeXmlTest::removeRemovesOnlyOurEntry()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    WriteAll(path, WithAddons(OtherToolAddon() + ClientAddon(QStringLiteral("False"))));

    QVERIFY(ExeXmlRemove(path, QLatin1String(kClientExe)));

    const QString content = ReadAll(path);

    QVERIFY(content.contains(QStringLiteral("other-tool.exe")));
    QVERIFY(content.contains(QStringLiteral("<Name>Other Tool</Name>")));
    QVERIFY(!content.contains(QStringLiteral("GSX-Integrator-Client.exe")));
    QCOMPARE(static_cast<int>(content.count(QStringLiteral("<Launch.Addon>"))), 1);
}

void ExeXmlTest::removeMissingFileIsSuccess()
{
    const QTemporaryDir dir;

    QVERIFY(ExeXmlRemove(dir.filePath(QStringLiteral("EXE.xml")), QLatin1String(kClientExe)));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("EXE.xml"))));
}

void ExeXmlTest::enabledEntryIsFoundCaseInsensitively()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    WriteAll(path, WithAddons(OtherToolAddon() + ClientAddon(QStringLiteral("False"))));

    QVERIFY(ExeXmlHasEnabledEntry(path, QLatin1String(kClientExe)));
}

void ExeXmlTest::disabledOrAbsentEntryIsNotEnabled()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("EXE.xml"));

    QVERIFY(!ExeXmlHasEnabledEntry(path, QLatin1String(kClientExe)));

    WriteAll(path, WithAddons(OtherToolAddon()));

    QVERIFY(!ExeXmlHasEnabledEntry(path, QLatin1String(kClientExe)));

    WriteAll(path, WithAddons(ClientAddon(QStringLiteral(" true "))));

    QVERIFY(!ExeXmlHasEnabledEntry(path, QLatin1String(kClientExe)));
}

void ExeXmlTest::candidatesAreTheFoldersThatExist()
{
    const QTemporaryDir home;
    QDir().mkpath(home.filePath(QStringLiteral("AppData/Roaming/Microsoft Flight Simulator 2024")));
    QDir().mkpath(home.filePath(QStringLiteral("AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache")));

    const std::vector<ExeXmlTarget> targets = CandidateExeXmlTargets(home.path());

    QCOMPARE(targets.size(), 2u);
    QCOMPARE(targets[0].label, QStringLiteral("MSFS 2020 (Microsoft Store)"));
    QCOMPARE(targets[0].path, QDir::toNativeSeparators(home.filePath(
                 QStringLiteral("AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/EXE.xml"))));
    QCOMPARE(targets[1].label, QStringLiteral("MSFS 2024 (Steam)"));
    QCOMPARE(targets[1].path, QDir::toNativeSeparators(home.filePath(
                 QStringLiteral("AppData/Roaming/Microsoft Flight Simulator 2024/EXE.xml"))));
}

QTEST_APPLESS_MAIN(ExeXmlTest)

#include "tst_exe_xml.moc"
