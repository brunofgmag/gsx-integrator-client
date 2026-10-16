#include <QtCore/QFile>
#include <QtTest/QTest>

#include "../src/infrastructure/update/GithubReleaseParser.h"

namespace
{
    QByteArray ReadFixture(const QString& name)
    {
        QFile file(QStringLiteral(GSX_FIXTURES_DIR "/") + name);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return file.readAll();
    }
}

class GithubReleaseParserTest final : public QObject
{
    Q_OBJECT

private slots:
    static void parsesLatestRelease();
    static void parsesTheAppImagePairWithoutMixingItWithTheZip();
    static void theAppImageNeverEndsUpInTheZipFields();
    static void rejectsMalformedJson();
    static void rejectsMissingTag();
    static void releaseWithoutClientAssetsStillParses();
    static void parsesSha256File();
    static void lowercasesSha256Hash();
    static void rejectsInvalidSha256Content();
    static void comparesVersions();
};

void GithubReleaseParserTest::parsesLatestRelease()
{
    const auto info = ParseLatestRelease(ReadFixture(QStringLiteral("github-latest-release.json")));

    QVERIFY(info.has_value());
    QCOMPARE(info->version, QStringLiteral("1.4.0"));
    QCOMPARE(info->releasePageUrl,
             QStringLiteral("https://github.com/brunofgmag/gsx-integrator-client/releases/tag/v1.4.0"));
    QCOMPARE(info->zipName, QStringLiteral("gsx-integrator-client-1.4.0.zip"));
    QVERIFY(info->zipUrl.endsWith(QStringLiteral("/gsx-integrator-client-1.4.0.zip")));
    QVERIFY(info->shaUrl.endsWith(QStringLiteral("/gsx-integrator-client-1.4.0.zip.sha256")));
}

namespace
{
    QByteArray ReleaseWithAssets(const QStringList& names)
    {
        QString assets;
        for (const QString& name : names)
        {
            assets += QStringLiteral("{\"name\":\"%1\",\"browser_download_url\":\"https://example.test/%1\"},").arg(name);
        }
        assets.chop(1);

        return QStringLiteral("{\"tag_name\":\"v2.0.0\",\"html_url\":\"https://example.test/r\",\"assets\":[%1]}")
            .arg(assets).toUtf8();
    }
}

void GithubReleaseParserTest::parsesTheAppImagePairWithoutMixingItWithTheZip()
{
    const auto info = ParseLatestRelease(ReleaseWithAssets({
        QStringLiteral("gsx-integrator-client-2.0.0.zip"),
        QStringLiteral("gsx-integrator-client-2.0.0.zip.sha256"),
        QStringLiteral("gsx-integrator-client-2.0.0-x86_64.AppImage"),
        QStringLiteral("gsx-integrator-client-2.0.0-x86_64.AppImage.sha256"),
        QStringLiteral("gsx-integrator-flightsim-to-2.0.0.zip")}));

    QVERIFY(info.has_value());
    QVERIFY(info->appImageUrl.endsWith(QStringLiteral("/gsx-integrator-client-2.0.0-x86_64.AppImage")));
    QVERIFY(info->appImageShaUrl.endsWith(QStringLiteral("/gsx-integrator-client-2.0.0-x86_64.AppImage.sha256")));
    QCOMPARE(info->zipName, QStringLiteral("gsx-integrator-client-2.0.0.zip"));
    QVERIFY(info->shaUrl.endsWith(QStringLiteral(".zip.sha256")));
}

void GithubReleaseParserTest::theAppImageNeverEndsUpInTheZipFields()
{
    const auto info = ParseLatestRelease(ReleaseWithAssets({
        QStringLiteral("gsx-integrator-client-2.0.0-x86_64.AppImage"),
        QStringLiteral("gsx-integrator-client-2.0.0-x86_64.AppImage.sha256")}));

    QVERIFY(info.has_value());
    QVERIFY(info->zipUrl.isEmpty());
    QVERIFY(info->zipName.isEmpty());
    QVERIFY(info->shaUrl.isEmpty());
    QVERIFY(!info->appImageUrl.isEmpty());
}

void GithubReleaseParserTest::rejectsMalformedJson()
{
    QVERIFY(!ParseLatestRelease("{not json").has_value());
    QVERIFY(!ParseLatestRelease("[]").has_value());
    QVERIFY(!ParseLatestRelease({}).has_value());
}

void GithubReleaseParserTest::rejectsMissingTag()
{
    QVERIFY(!ParseLatestRelease("{\"html_url\": \"https://example.com\"}").has_value());
    QVERIFY(!ParseLatestRelease("{\"tag_name\": \"  \"}").has_value());
}

void GithubReleaseParserTest::releaseWithoutClientAssetsStillParses()
{
    const auto info = ParseLatestRelease(ReadFixture(QStringLiteral("github-commbus-release.json")));

    QVERIFY(info.has_value());
    QCOMPARE(info->version, QStringLiteral("0.3.0"));
    QCOMPARE(info->releasePageUrl,
             QStringLiteral("https://github.com/brunofgmag/gsx-integrator-commbus/releases/tag/v0.3.0"));
    QVERIFY(info->zipUrl.isEmpty());
    QVERIFY(info->shaUrl.isEmpty());
}

void GithubReleaseParserTest::parsesSha256File()
{
    const QByteArray hash(64, 'a');

    QCOMPARE(ParseSha256File(hash), QString::fromUtf8(hash));
    QCOMPARE(ParseSha256File(hash + "  gsx-integrator-client-1.4.0.zip"), QString::fromUtf8(hash));
    QCOMPARE(ParseSha256File(hash + "  file.zip\n"), QString::fromUtf8(hash));
}

void GithubReleaseParserTest::lowercasesSha256Hash()
{
    const QByteArray hash(64, 'A');

    QCOMPARE(ParseSha256File(hash), QString::fromUtf8(QByteArray(64, 'a')));
}

void GithubReleaseParserTest::rejectsInvalidSha256Content()
{
    QVERIFY(ParseSha256File({}).isEmpty());
    QVERIFY(ParseSha256File("not a hash").isEmpty());
    QVERIFY(ParseSha256File(QByteArray(63, 'a')).isEmpty());
    QVERIFY(ParseSha256File(QByteArray(64, 'g')).isEmpty());
}

void GithubReleaseParserTest::comparesVersions()
{
    QVERIFY(IsNewerVersion(QStringLiteral("v1.4.0"), QStringLiteral("1.3.0")));
    QVERIFY(IsNewerVersion(QStringLiteral("1.4.0"), QStringLiteral("1.3.9")));
    QVERIFY(IsNewerVersion(QStringLiteral("v2.0.0"), QStringLiteral("v1.9.9")));
    QVERIFY(!IsNewerVersion(QStringLiteral("v1.4.0"), QStringLiteral("1.4.0")));
    QVERIFY(!IsNewerVersion(QStringLiteral("v1.2.0"), QStringLiteral("1.3.0")));
    QVERIFY(!IsNewerVersion(QStringLiteral("nightly"), QStringLiteral("1.3.0")));
    QVERIFY(!IsNewerVersion(QStringLiteral("v1.4.0"), QString()));
}

QTEST_APPLESS_MAIN(GithubReleaseParserTest)

#include "tst_github_release_parser.moc"
