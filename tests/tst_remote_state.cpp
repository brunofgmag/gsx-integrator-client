#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <QtTest/QTest>

#include <algorithm>
#include <vector>
#include "RecordedWire.h"
#include "../src/infrastructure/gsx/GsxRemoteState.h"
#include "../src/infrastructure/gsx/GsxRemoteStateReducer.h"

namespace
{
    constexpr auto kWireLfmnFirstProcess = "wire-key-20261002-175856.jsonl";
    constexpr auto kWireLfmnSecondProcess = "wire-key-20261002-183503.jsonl";
    constexpr auto kWireNewbornCouatl = "wire-key-20261002-184537.jsonl";
    constexpr auto kWireCdk2Flight = "wire-key-20261003-094025.jsonl";
    constexpr auto kSnapshotLfmnFirstProcess = "2026-10-02T17:58:57.345";
    constexpr auto kSnapshotLfmnSecondProcess = "2026-10-02T18:35:05.485";
    constexpr auto kSnapshotNewbornCouatl = "2026-10-02T18:46:08.901";
    constexpr auto kSnapshotCdk2Flight = "2026-10-03T09:40:26.093";
    constexpr qint64 kSidAboveInt32 = 3000000001;

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

    std::vector<RecordedMessage> Wire(const char* const fileName)
    {
        return LoadRecordedWire(QLatin1String(fileName));
    }

    void ApplyRecorded(GsxRemoteState& state, const RecordedMessage& recorded)
    {
        if (recorded.Type() == QStringLiteral("snapshot"))
        {
            GsxRemoteStateReducer::ApplySnapshot(state, recorded.message);
        }
        else if (recorded.Type() == QStringLiteral("patch"))
        {
            GsxRemoteStateReducer::ApplyPatch(state, recorded.Path().toStdString(), recorded.Value());
        }
    }

    bool WireHasStamp(const std::vector<RecordedMessage>& wire, const QString& stamp)
    {
        return std::ranges::any_of(wire, [&stamp](const RecordedMessage& recorded)
        {
            return recorded.stamp == stamp;
        });
    }

    GsxRemoteState StateAfterStamp(const std::vector<RecordedMessage>& wire, const QString& lastStamp)
    {
        GsxRemoteState state;
        if (!WireHasStamp(wire, lastStamp))
        {
            QTest::qFail(qPrintable(QStringLiteral("no line of the wire is stamped ") + lastStamp),
                         __FILE__, __LINE__);

            return state;
        }

        for (const RecordedMessage& recorded : wire)
        {
            if (recorded.stamp > lastStamp)
            {
                break;
            }
            ApplyRecorded(state, recorded);
        }

        return state;
    }

    GsxRemoteState LoadedState()
    {
        GsxRemoteState state;
        GsxRemoteStateReducer::ApplyConnection(state, true);

        for (const RecordedMessage& recorded : Wire(kWireLfmnFirstProcess))
        {
            ApplyRecorded(state, recorded);
        }

        const QJsonArray fixtures = LoadFixtures();
        ApplySnapshots(state, fixtures);
        ApplyPatches(state, fixtures);
        ApplyMenuPatches(state, fixtures);
        GsxRemoteStateReducer::ApplyPatch(
            state, "/simbrief", QJsonDocument::fromJson(R"({"status":"ok","error":"none","gen":7})").object());
        GsxRemoteStateReducer::ApplyPatch(state, "/menuShown", QJsonValue(true));

        return state;
    }

    void VerifyEveryFieldIsHeld(const GsxRemoteState& state)
    {
        QVERIFY(state.connected);
        QVERIFY(state.synced);
        QVERIFY(!state.simbriefStatus.empty());
        QVERIFY(!state.simbriefError.empty());
        QCOMPARE(state.simbriefGeneration, 7);
        QCOMPARE(state.handlingOperator, std::string{"Operator A"});
        QCOMPARE(state.matchedAircraftTitle, std::string{"TFDI MD11"});
        QCOMPARE(state.couatlId, std::string{"2120420471"});
        QVERIFY(!state.airportIcao.empty());
        QVERIFY(!state.parkingName.empty());
        QVERIFY(!state.apronVerdict.empty());
        QVERIFY(!state.menu.title.empty());
        QVERIFY(!state.menu.entries.empty());
        QVERIFY(!state.menu.disabled.empty());
        QVERIFY(state.menu.shown);
        QVERIFY(!state.services.empty());
    }

    void VerifyNothingIsHeld(const GsxRemoteState& state)
    {
        QVERIFY(state.connected);
        QVERIFY(!state.synced);
        QVERIFY(state.simbriefStatus.empty());
        QVERIFY(state.simbriefError.empty());
        QCOMPARE(state.simbriefGeneration, 0);
        QVERIFY(state.handlingOperator.empty());
        QVERIFY(state.matchedAircraftTitle.empty());
        QVERIFY(state.couatlId.empty());
        QVERIFY(state.airportIcao.empty());
        QVERIFY(state.parkingName.empty());
        QVERIFY(state.apronVerdict.empty());
        QVERIFY(state.menu.title.empty());
        QVERIFY(state.menu.entries.empty());
        QVERIFY(state.menu.disabled.empty());
        QVERIFY(!state.menu.shown);
        QVERIFY(state.services.empty());
    }

    const RecordedMessage* FirstPatchTo(const std::vector<RecordedMessage>& wire, const QString& path)
    {
        const auto found = std::ranges::find_if(wire, [&path](const RecordedMessage& recorded)
        {
            return recorded.Type() == QStringLiteral("patch") && recorded.Path() == path;
        });

        return found == wire.end() ? nullptr : &*found;
    }

    QStringList KeyOf(const GsxRemoteState& state)
    {
        return {QString::fromStdString(state.couatlId),
                QString::fromStdString(state.airportIcao),
                QString::fromStdString(state.parkingName)};
    }

    void AddKeyRow(const char* const name, const char* const file, const char* const until,
                   const char* const couatl, const char* const airport, const char* const parking)
    {
        QTest::newRow(name) << QString::fromLatin1(file) << QString::fromLatin1(until)
                            << QString::fromUtf8(couatl) << QString::fromUtf8(airport)
                            << QString::fromUtf8(parking);
    }

    void AddKeyColumns()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("until");
        QTest::addColumn<QString>("couatl");
        QTest::addColumn<QString>("airport");
        QTest::addColumn<QString>("parking");
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
    static void theRecordedWiresReproduceTheMeasuredKey_data();
    static void theRecordedWiresReproduceTheMeasuredKey();
    static void theKeyPathsOfTheRecordedWiresAreNoLongerUnknown();
    static void aStartupPatchWithoutASidKeepsTheCouatlId();
    static void aStartupPatchWithAnotherSidReplacesTheCouatlId();
    static void aSidAboveTheInt32LimitIsKeptExactly();
    static void aSidSentAsTextIsKeptAsItIs();
    static void aSidThatIsNotAnIdentifierKeepsTheCouatlId_data();
    static void aSidThatIsNotAnIdentifierKeepsTheCouatlId();
    static void aSidOfZeroOrBelowStaysAnIdentifier_data();
    static void aSidOfZeroOrBelowStaysAnIdentifier();
    static void aDropOnlyLowersTheConnectedFlag();
    static void theHandshakeAfterADropEmptiesEveryField();
    static void aSnapshotSyncsTheState_data();
    static void aSnapshotSyncsTheState();
    static void aPatchBeforeTheSnapshotDoesNotSyncTheState();
    static void aRecordedLineEndingInCarriageReturnAndLineFeedIsParsed();
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

void RemoteStateTest::theRecordedWiresReproduceTheMeasuredKey_data()
{
    AddKeyColumns();

    AddKeyRow("094025 after the snapshot", kWireCdk2Flight, kSnapshotCdk2Flight,
              "29495244", "CDK2", "Parking 2");
    AddKeyRow("094025 parking null at take-off", kWireCdk2Flight, "2026-10-03T10:32:48.287",
              "29495244", "CDK2", "");
    AddKeyRow("094025 airport null in the air", kWireCdk2Flight, "2026-10-03T10:33:39.329",
              "29495244", "", "");
    AddKeyRow("094025 airport CYEG", kWireCdk2Flight, "2026-10-03T11:55:39.127",
              "29495244", "CYEG", "");
    AddKeyRow("094025 airport null again", kWireCdk2Flight, "2026-10-03T11:57:22.087",
              "29495244", "", "");
    AddKeyRow("094025 airport CYEG again", kWireCdk2Flight, "2026-10-03T12:05:19.073",
              "29495244", "CYEG", "");
    AddKeyRow("094025 parking Gate 8", kWireCdk2Flight, "2026-10-03T12:11:11.102",
              "29495244", "CYEG", "Gate 8");

    AddKeyRow("175856 after the snapshot", kWireLfmnFirstProcess, kSnapshotLfmnFirstProcess,
              "2120420471", "LFMN", "Terminal 1 | Gate C14");
    AddKeyRow("183503 after the snapshot", kWireLfmnSecondProcess, kSnapshotLfmnSecondProcess,
              "2120420471", "LFMN", "Terminal 1 | Gate C14");

    AddKeyRow("184537 newborn snapshot", kWireNewbornCouatl, kSnapshotNewbornCouatl, "", "", "");
    AddKeyRow("184537 airport by patch", kWireNewbornCouatl, "2026-10-02T18:46:10.940",
              "", "LFMN", "");
    AddKeyRow("184537 sid by startup patch", kWireNewbornCouatl, "2026-10-02T18:46:10.957",
              "2123650379", "LFMN", "");
    AddKeyRow("184537 parking by patch", kWireNewbornCouatl, "2026-10-02T18:46:12.160",
              "2123650379", "LFMN", "Terminal 1 | Gate C14");
    AddKeyRow("184537 end of the wire", kWireNewbornCouatl, "2026-10-02T18:46:16.123",
              "2123650379", "LFMN", "Terminal 1 | Gate C14");
}

void RemoteStateTest::theRecordedWiresReproduceTheMeasuredKey()
{
    QFETCH(QString, file);
    QFETCH(QString, until);
    QFETCH(QString, couatl);
    QFETCH(QString, airport);
    QFETCH(QString, parking);

    const std::vector<RecordedMessage> wire = LoadRecordedWire(file);

    QVERIFY(!wire.empty());

    const GsxRemoteState state = StateAfterStamp(wire, until);

    QCOMPARE(KeyOf(state), (QStringList{couatl, airport, parking}));
}

void RemoteStateTest::theKeyPathsOfTheRecordedWiresAreNoLongerUnknown()
{
    int keyPatches = 0;
    for (const char* const file : {kWireLfmnFirstProcess, kWireLfmnSecondProcess, kWireNewbornCouatl, kWireCdk2Flight})
    {
        const std::vector<RecordedMessage> wire = Wire(file);

        QVERIFY(!wire.empty());

        for (const RecordedMessage& recorded : wire)
        {
            if (recorded.Type() != QStringLiteral("patch"))
            {
                continue;
            }

            GsxRemoteState state;
            const GsxPatchOutcome outcome =
                GsxRemoteStateReducer::ApplyPatch(state, recorded.Path().toStdString(), recorded.Value());

            QVERIFY2(outcome != GsxPatchOutcome::Unknown, qPrintable(recorded.Path()));
            ++keyPatches;
        }
    }

    QCOMPARE(keyPatches, 12);
}

void RemoteStateTest::aStartupPatchWithoutASidKeepsTheCouatlId()
{
    const std::vector<RecordedMessage> flight = Wire(kWireCdk2Flight);
    const std::vector<RecordedMessage> newborn = Wire(kWireNewbornCouatl);
    const RecordedMessage* const startup = FirstPatchTo(newborn, QStringLiteral("/startup"));

    QVERIFY(!flight.empty());
    QVERIFY(startup != nullptr);

    QJsonObject withoutSid = startup->Value().toObject();
    QVERIFY(withoutSid.contains("sid"));
    withoutSid.remove("sid");

    GsxRemoteState state = StateAfterStamp(flight, QString::fromLatin1(kSnapshotCdk2Flight));

    QCOMPARE(state.couatlId, std::string{"29495244"});

    GsxRemoteStateReducer::ApplyPatch(state, "/startup", withoutSid);

    QCOMPARE(state.couatlId, std::string{"29495244"});
}

void RemoteStateTest::aStartupPatchWithAnotherSidReplacesTheCouatlId()
{
    const std::vector<RecordedMessage> flight = Wire(kWireCdk2Flight);
    const std::vector<RecordedMessage> newborn = Wire(kWireNewbornCouatl);
    const RecordedMessage* const startup = FirstPatchTo(newborn, QStringLiteral("/startup"));

    QVERIFY(!flight.empty());
    QVERIFY(startup != nullptr);

    GsxRemoteState state = StateAfterStamp(flight, QString::fromLatin1(kSnapshotCdk2Flight));

    QCOMPARE(state.couatlId, std::string{"29495244"});

    GsxRemoteStateReducer::ApplyPatch(state, "/startup", startup->Value());

    QCOMPARE(state.couatlId, std::string{"2123650379"});
}

void RemoteStateTest::aSidAboveTheInt32LimitIsKeptExactly()
{
    QJsonObject startup;
    startup.insert(QStringLiteral("sid"), QJsonValue(kSidAboveInt32));

    GsxRemoteState state;

    GsxRemoteStateReducer::ApplyPatch(state, "/startup", startup);

    QCOMPARE(state.couatlId, std::string{"3000000001"});
}

void RemoteStateTest::aSidSentAsTextIsKeptAsItIs()
{
    QJsonObject startup;
    startup.insert(QStringLiteral("sid"), QStringLiteral("c0ffee"));

    GsxRemoteState state;

    GsxRemoteStateReducer::ApplyPatch(state, "/startup", startup);

    QCOMPARE(state.couatlId, std::string{"c0ffee"});
}

void RemoteStateTest::aSidThatIsNotAnIdentifierKeepsTheCouatlId_data()
{
    QTest::addColumn<QJsonValue>("sid");

    QTest::newRow("a fraction") << QJsonValue(2.5);
    QTest::newRow("a value beyond int64") << QJsonValue(1e30);
    QTest::newRow("an empty string") << QJsonValue(QString());
    QTest::newRow("null") << QJsonValue();
}

void RemoteStateTest::aSidThatIsNotAnIdentifierKeepsTheCouatlId()
{
    QFETCH(QJsonValue, sid);

    const std::vector<RecordedMessage> flight = Wire(kWireCdk2Flight);

    QVERIFY(!flight.empty());

    GsxRemoteState state = StateAfterStamp(flight, QString::fromLatin1(kSnapshotCdk2Flight));

    QCOMPARE(state.couatlId, std::string{"29495244"});

    QJsonObject startup;
    startup.insert(QStringLiteral("sid"), sid);
    GsxRemoteStateReducer::ApplyPatch(state, "/startup", startup);

    QCOMPARE(state.couatlId, std::string{"29495244"});
}

void RemoteStateTest::aSidOfZeroOrBelowStaysAnIdentifier_data()
{
    QTest::addColumn<QJsonValue>("sid");
    QTest::addColumn<QString>("expected");

    QTest::newRow("zero") << QJsonValue(0.0) << QStringLiteral("0");
    QTest::newRow("negative") << QJsonValue(-42.0) << QStringLiteral("-42");
}

void RemoteStateTest::aSidOfZeroOrBelowStaysAnIdentifier()
{
    QFETCH(QJsonValue, sid);
    QFETCH(QString, expected);

    const std::vector<RecordedMessage> flight = Wire(kWireCdk2Flight);

    QVERIFY(!flight.empty());

    GsxRemoteState state = StateAfterStamp(flight, QString::fromLatin1(kSnapshotCdk2Flight));
    QJsonObject startup;
    startup.insert(QStringLiteral("sid"), sid);
    GsxRemoteStateReducer::ApplyPatch(state, "/startup", startup);

    QCOMPARE(QString::fromStdString(state.couatlId), expected);
}

void RemoteStateTest::aDropOnlyLowersTheConnectedFlag()
{
    GsxRemoteState state = LoadedState();

    VerifyEveryFieldIsHeld(state);

    GsxRemoteStateReducer::ApplyConnection(state, false);

    QVERIFY(!state.connected);

    state.connected = true;

    VerifyEveryFieldIsHeld(state);
}

void RemoteStateTest::theHandshakeAfterADropEmptiesEveryField()
{
    GsxRemoteState state = LoadedState();

    VerifyEveryFieldIsHeld(state);

    GsxRemoteStateReducer::ApplyConnection(state, false);
    GsxRemoteStateReducer::ApplyConnection(state, true);

    VerifyNothingIsHeld(state);
}

void RemoteStateTest::aSnapshotSyncsTheState_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("175856") << QString::fromLatin1(kWireLfmnFirstProcess);
    QTest::newRow("183503") << QString::fromLatin1(kWireLfmnSecondProcess);
    QTest::newRow("184537 newborn") << QString::fromLatin1(kWireNewbornCouatl);
    QTest::newRow("094025") << QString::fromLatin1(kWireCdk2Flight);
}

void RemoteStateTest::aSnapshotSyncsTheState()
{
    QFETCH(QString, file);

    const std::vector<RecordedMessage> wire = LoadRecordedWire(file);

    QVERIFY(!wire.empty());

    GsxRemoteState state;
    GsxRemoteStateReducer::ApplyConnection(state, true);

    QVERIFY(!state.synced);

    for (const RecordedMessage& recorded : wire)
    {
        ApplyRecorded(state, recorded);
        if (recorded.Type() == QStringLiteral("snapshot"))
        {
            break;
        }

        QVERIFY(!state.synced);
    }

    QVERIFY(state.synced);
}

void RemoteStateTest::aPatchBeforeTheSnapshotDoesNotSyncTheState()
{
    GsxRemoteState state;
    GsxRemoteStateReducer::ApplyConnection(state, true);

    GsxRemoteStateReducer::ApplyPatch(state, "/parking", QJsonValue(QStringLiteral("Gate 8")));

    QCOMPARE(state.parkingName, std::string{"Gate 8"});
    QVERIFY(!state.synced);
}

void RemoteStateTest::aRecordedLineEndingInCarriageReturnAndLineFeedIsParsed()
{
    const QByteArray text = QByteArrayLiteral(
        "2026-10-03T09:40:26.074 [NoSession] {\"type\": \"hello\"}\r\n"
        "2026-10-03T09:40:26.093 [NoSession] {\"type\": \"snapshot\"}\r\n");

    const std::vector<RecordedMessage> wire = ParseRecordedWire(text);

    QCOMPARE(wire.size(), std::size_t{2});
    QCOMPARE(wire[0].stamp, QStringLiteral("2026-10-03T09:40:26.074"));
    QCOMPARE(wire[0].Type(), QStringLiteral("hello"));
    QCOMPARE(wire[1].stamp, QStringLiteral("2026-10-03T09:40:26.093"));
    QCOMPARE(wire[1].Type(), QStringLiteral("snapshot"));
}

QTEST_APPLESS_MAIN(RemoteStateTest)

#include "tst_remote_state.moc"
