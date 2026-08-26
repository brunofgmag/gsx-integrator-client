#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtTest/QTest>

#include <algorithm>
#include "../src/infrastructure/gsx/GsxRemoteState.h"
#include "../src/infrastructure/gsx/GsxRemoteStateReducer.h"

namespace
{
    QJsonArray LoadFixtures()
    {
        QFile file(QStringLiteral(GSX_FIXTURES_DIR) + QStringLiteral("/remoteapi-fixtures.json"));
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return QJsonDocument::fromJson(file.readAll()).array();
    }

    QJsonObject MessageFromEvent(const QJsonValue& event)
    {
        return event.toObject().value("msg").toObject();
    }

    bool Contains(const std::vector<std::string>& values, const std::string& expected)
    {
        return std::find(values.begin(), values.end(), expected) != values.end();
    }

    void ApplySnapshots(GsxRemoteState& state, const QJsonArray& fixtures)
    {
        for (const QJsonValue& event : fixtures)
        {
            const QJsonObject message = MessageFromEvent(event);
            if (message.value("type").toString() == QStringLiteral("snapshot"))
            {
                GsxRemoteStateReducer::ApplySnapshot(state, message);
            }
        }
    }

    void ApplyPatches(GsxRemoteState& state, const QJsonArray& fixtures)
    {
        for (const QJsonValue& event : fixtures)
        {
            const QJsonObject message = MessageFromEvent(event);
            if (message.value("type").toString() == QStringLiteral("patch"))
            {
                GsxRemoteStateReducer::ApplyPatch(state,
                                                  message.value("path").toString().toStdString(),
                                                  message.value("value"));
            }
        }
    }

    void ApplyMenuPatches(GsxRemoteState& state, const QJsonArray& fixtures)
    {
        for (const QJsonValue& event : fixtures)
        {
            const QJsonObject message = MessageFromEvent(event);
            if (message.value("type").toString() == QStringLiteral("patch") &&
                message.value("path").toString() == QStringLiteral("/menu"))
            {
                GsxRemoteStateReducer::ApplyPatch(state, "/menu", message.value("value"));
            }
        }
    }
}

class RemoteStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void snapshotPopulatesServicesAndMenu();
    static void menuPatchFillsEntries();
    static void everyPatchPathInTheCaptureIsAccountedFor();
    static void discardedPathsAreNamedAndNotSilent();
    static void theHandlingOperatorIsReadFromTheCapture();
    static void theApronVerdictIsReadFromTheCapture();
    static void theMatchedAircraftTitleIsReadFromTheCapture();
    static void theSimbriefGenerationIsReadFromTheCapture();
};

void RemoteStateTest::snapshotPopulatesServicesAndMenu()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    ApplySnapshots(state, fixtures);

    QCOMPARE(state.services.size(), std::size_t{12});

    const auto* gpu = FindService(state, "GPU");

    QVERIFY(gpu);
    QCOMPARE(gpu->stateRaw, 5);
    QVERIFY(!gpu->canTrigger);
}

void RemoteStateTest::menuPatchFillsEntries()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    ApplyMenuPatches(state, fixtures);

    QVERIFY(Contains(state.menu.entries, "Reposition Aircraft"));
    QVERIFY(state.menu.title.find("Activate Services") != std::string::npos);
}

void RemoteStateTest::everyPatchPathInTheCaptureIsAccountedFor()
{
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    QStringList unknown;
    for (const QJsonValue& event : fixtures)
    {
        const QJsonObject message = MessageFromEvent(event);
        if (message.value("type").toString() != QStringLiteral("patch"))
        {
            continue;
        }

        GsxRemoteState state;
        const std::string path = message.value("path").toString().toStdString();
        if (GsxRemoteStateReducer::ApplyPatch(state, path, message.value("value"))
            == GsxPatchOutcome::Unknown)
        {
            unknown.append(QString::fromStdString(path));
        }
    }

    QVERIFY2(unknown.isEmpty(), qPrintable(unknown.join(QLatin1Char(' '))));
}

void RemoteStateTest::discardedPathsAreNamedAndNotSilent()
{
    GsxRemoteState state;

    QCOMPARE(GsxRemoteStateReducer::ApplyPatch(state, "/billing", QJsonValue()),
             GsxPatchOutcome::Discarded);
    QCOMPARE(GsxRemoteStateReducer::ApplyPatch(state, "/message", QJsonValue()),
             GsxPatchOutcome::Discarded);
    QCOMPARE(GsxRemoteStateReducer::ApplyPatch(state, "/menuShown", QJsonValue(true)),
             GsxPatchOutcome::Applied);
    QCOMPARE(GsxRemoteStateReducer::ApplyPatch(state, "/somethingNobodyHasSeen", QJsonValue()),
             GsxPatchOutcome::Unknown);
}

void RemoteStateTest::theHandlingOperatorIsReadFromTheCapture()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());
    QVERIFY(state.handlingOperator.empty());

    ApplyPatches(state, fixtures);

    QCOMPARE(state.handlingOperator, std::string{"Operator A"});
}

void RemoteStateTest::theApronVerdictIsReadFromTheCapture()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    ApplySnapshots(state, fixtures);

    QCOMPARE(state.apronVerdict.size(), std::size_t{2});
    QVERIFY(Contains(state.apronVerdict, "Ramp Cargo"));
    QVERIFY(Contains(state.apronVerdict, "max wingspan 66m"));
}

void RemoteStateTest::theMatchedAircraftTitleIsReadFromTheCapture()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    ApplySnapshots(state, fixtures);

    QCOMPARE(state.matchedAircraftTitle, std::string{"TFDI MD11"});
}

void RemoteStateTest::theSimbriefGenerationIsReadFromTheCapture()
{
    GsxRemoteState state;
    const QJsonArray fixtures = LoadFixtures();

    QVERIFY(!fixtures.isEmpty());

    ApplySnapshots(state, fixtures);

    const int fromSnapshot = state.simbriefGeneration;

    GsxRemoteStateReducer::ApplyPatch(state, "/simbrief",
                                      QJsonDocument::fromJson(R"({"status":"ok","gen":7})").object());

    QCOMPARE(fromSnapshot, 0);
    QCOMPARE(state.simbriefGeneration, 7);
}

QTEST_APPLESS_MAIN(RemoteStateTest)

#include "tst_remote_state.moc"
