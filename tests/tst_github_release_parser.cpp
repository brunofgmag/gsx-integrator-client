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
