#include <algorithm>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/checkpoint/JsonFileTurnaroundCheckpointStore.h"
#include "doubles/FakeTurnaroundCheckpointStore.h"
#include "turnaround/TurnaroundDataFill.h"

namespace
{
    constexpr auto kClientVersion = "1.44.0";
    constexpr auto kFileName = "turnaround.json";
    constexpr qint64 kSizeCap = 1024 * 1024;

    using Edit = std::function<void(QJsonObject&)>;

    class LogCapture
    {
    public:
        LogCapture() : previous_(qInstallMessageHandler(Collect)) { Clear(); }
        ~LogCapture() { qInstallMessageHandler(previous_); }

        LogCapture(const LogCapture&) = delete;
        LogCapture& operator=(const LogCapture&) = delete;

        static void Clear() { Lines().clear(); }

        [[nodiscard]] static int Count() { return static_cast<int>(Lines().size()); }

        [[nodiscard]] static bool Contains(const char* const fragment)
        {
            return std::ranges::any_of(Lines(), [fragment](const QString& line) { return line.contains(QLatin1String(fragment)); });
        }

    private:
        static QStringList& Lines()
        {
            static QStringList lines;

            return lines;
        }

        static void Collect(QtMsgType, const QMessageLogContext&, const QString& message) { Lines().append(message); }

        QtMessageHandler previous_;
    };

    TurnaroundData EveryRawLeafFilled()
    {
        TurnaroundData data;
        turnaround::VisitFields(
            [&data](const char*, const auto accessor, const turnaround::FieldRestore restore)
            {
                if (restore == turnaround::FieldRestore::Raw)
                {
                    MakeNonDefault(accessor(data));
                }
            });

        return data;
    }

    TurnaroundData WithAlternatingRawBools(const TurnaroundData& source)
    {
        TurnaroundData data = source;
        int index = 0;
        turnaround::VisitFields(
            [&data, &index](const char*, const auto accessor, [[maybe_unused]] const turnaround::FieldRestore restore)
            {
                if constexpr (std::same_as<std::remove_cvref_t<decltype(accessor(data))>, bool>)
                {
                    if (restore == turnaround::FieldRestore::Raw)
                    {
                        accessor(data) = index++ % 2 == 0;
                    }
                }
            });

        return data;
    }

    std::vector<QString> LeafNames(const bool raw)
    {
        std::vector<QString> names;
        turnaround::VisitFields(
            [&names, raw](const char* const name, const auto, const turnaround::FieldRestore restore)
            {
                if ((restore == turnaround::FieldRestore::Raw) == raw)
                {
                    names.push_back(QString::fromLatin1(name));
                }
            });

        return names;
    }

    int RawLeavesLeftAtTheirDefault(const TurnaroundData& data)
    {
        const TurnaroundData defaults;
        int unchanged = 0;
        turnaround::VisitFields(
            [&data, &defaults, &unchanged](const char*, const auto accessor, const turnaround::FieldRestore restore)
            {
                if (restore == turnaround::FieldRestore::Raw && accessor(data) == accessor(defaults))
                {
                    ++unchanged;
                }
            });

        return unchanged;
    }

    int NotRawLeavesDifferingFromTheirDefault(const TurnaroundData& data)
    {
        const TurnaroundData defaults;
        int differing = 0;
        turnaround::VisitFields(
            [&data, &defaults, &differing](const char*, const auto accessor, const turnaround::FieldRestore restore)
            {
                if (restore != turnaround::FieldRestore::Raw && !(accessor(data) == accessor(defaults)))
                {
                    ++differing;
                }
            });

        return differing;
    }

    int NotRawFilledLeavesFoundIn(const QJsonObject& written)
    {
        const TurnaroundData filled = EveryFieldNonDefault();
        const TurnaroundData defaults;
        int found = 0;
        turnaround::VisitFields(
            [&written, &filled, &defaults, &found](const char* const name, const auto accessor, const turnaround::FieldRestore restore)
            {
                if (restore != turnaround::FieldRestore::Raw && !(accessor(filled) == accessor(defaults)) &&
                    written.contains(QLatin1StringView(name)))
                {
                    ++found;
                }
            });

        return found;
    }

    TurnaroundData WithTheNotRawLeavesAtTheirDefault(const TurnaroundData& source)
    {
        TurnaroundData data = source;
        const TurnaroundData defaults;
        turnaround::VisitFields(
            [&data, &defaults](const char*, const auto accessor, const turnaround::FieldRestore restore)
            {
                if (restore != turnaround::FieldRestore::Raw)
                {
                    accessor(data) = accessor(defaults);
                }
            });

        return data;
    }

    MemoryBag BagWith(const std::string& name, const std::string& value)
    {
        MemoryBag bag;
        bag.PutText(name, value);

        return bag;
    }

    TurnaroundKey SampleKey()
    {
        TurnaroundKey key;
        key.couatlId = "2120420471";
        key.aircraftId = "fss-727-200f";
        key.aircraftTitle = "FSS Boeing 727-200F";
        key.airportIcao = "LFMN";
        key.parkingName = "Terminal 1 | Gate C14";

        return key;
    }

    FlightPlan SamplePlan()
    {
        FlightPlan plan;
        plan.fuelKg = 12345.678;
        plan.zfwKg = 0.1;
        plan.passengers = 148;
        plan.unit = WeightUnit::Lb;
        plan.origin = "LFMN";
        plan.destination = "CYEG";
        plan.generatedEpoch = 1759478400;
        plan.operatingEmptyKg = 42100.5;
        plan.payloadKg = 18000.25;
        plan.cargoKg = 2400.0;

        return plan;
    }

    TurnaroundDocument FullDocument()
    {
        TurnaroundCheckpoint checkpoint;
        checkpoint.phase = TurnaroundPhase::Loading;
        checkpoint.data = EveryRawLeafFilled();
        checkpoint.aircraftMemory.PutText("door.front", "open");
        checkpoint.aircraftMemory.PutNumber("deck", 0.75);
        checkpoint.aircraftMemory.PutFlag("closeRequested", true);

        TurnaroundDocument document;
        document.key = SampleKey();
        document.checkpoint = checkpoint;
        document.plan = SamplePlan();
        document.memoryByOwner.emplace("gsx", BagWith("completed", "1"));
        document.memoryByOwner.emplace("doors", BagWith("cargo", "open"));
        document.repositioned = true;

        return document;
    }

    TurnaroundDocument KeyAndMarkOnly()
    {
        TurnaroundDocument document;
        document.key = SampleKey();
        document.repositioned = true;

        return document;
    }

    QByteArray ReadAll(const QString& path)
    {
        QFile file(path);

        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    bool WriteAll(const QString& path, const QByteArray& bytes)
    {
        QFile file(path);

        return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
    }

    bool RewriteJson(const QString& path, const Edit& edit)
    {
        QJsonObject root = QJsonDocument::fromJson(ReadAll(path)).object();
        edit(root);

        return WriteAll(path, QJsonDocument(root).toJson());
    }

    Edit Insert(const QString& key, const QJsonValue& value)
    {
        return [key, value](QJsonObject& object) { object.insert(key, value); };
    }

    Edit Remove(const QString& key)
    {
        return [key](QJsonObject& object) { object.remove(key); };
    }

    Edit Within(const QString& member, const Edit& edit)
    {
        return [member, edit](QJsonObject& object)
        {
            QJsonObject inner = object.value(member).toObject();
            edit(inner);
            object.insert(member, inner);
        };
    }

    Edit InData(const Edit& edit)
    {
        return Within(QStringLiteral("checkpoint"), Within(QStringLiteral("data"), edit));
    }

    Edit InPlan(const Edit& edit)
    {
        return Within(QStringLiteral("plan"), edit);
    }

    QJsonArray BagItems(const QJsonValue& item)
    {
        return QJsonArray{item};
    }

    void AddRefusal(const char* const name, const Edit& edit, const char* const reason)
    {
        QTest::newRow(name) << edit << QString::fromLatin1(reason);
    }

    bool RefusedBecause(const JsonFileTurnaroundCheckpointStore& store, const char* const fragment)
    {
        LogCapture::Clear();

        return !store.Read().has_value() && LogCapture::Count() == 1 && LogCapture::Contains(fragment);
    }
}

class JsonFileTurnaroundCheckpointStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() const;
    void init();
    void cleanup();

    void theFillHelperChangesEveryRawLeaf() const;
    void readOfAnAbsentFileIsNoneAndSilent() const;
    void everyRawLeafRoundTripsThroughARealDirectory();
    void aFullDocumentIsFarBelowTheSizeCap();
    void writeCreatesTheDirectoryOnFirstUse();
    void everyResumablePhaseRoundTrips();
    void onlyTheLeavesClassedRawAreWritten();
    void aLeafClassedNotRawInTheFileReadsAtItsDefault();
    void aDocumentWithNoCheckpointRoundTrips();
    void aRepositionedMarkOfFalseRoundTrips();
    void aDefaultDataCheckpointRoundTrips();
    void aMixOfTrueAndFalseRawBoolsRoundTrips();
    void aPlanWithoutPayloadOrCargoRoundTrips();
    void aPlanInKilogramsRoundTrips();
    void accentsBarsQuotesAndNewlinesSurvive();
    void aTitleWithAByteOutsideUtf8IsReadBackWithTheReplacementCharacter();
    void doublesExactAndInexactRoundTrip();
    void aNonFiniteRawDoubleReadsAsNoneOnTheNextRead();
    void aTruncatedFileReadsAsNone();
    void anotherClientVersionReadsAsNone();
    void anotherPhaseCountReadsAsNone();
    void anUnknownPhaseNameReadsAsNone();
    void aPhaseOutsideTheResumableRangeReadsAsNone_data() const;
    void aPhaseOutsideTheResumableRangeReadsAsNone();
    void jsonOfAnotherShapeReadsAsNone_data() const;
    void jsonOfAnotherShapeReadsAsNone();
    void aMalformedDocumentReadsAsNone_data() const;
    void aMalformedDocumentReadsAsNone();
    void aFileLargerThanTheCapReadsAsNone();
    void aDirectoryAtThePathReadsAsNoneAndCannotBeErased();
    void nullContainersReadAsAbsent();
    void anAbsentPlanLeafReadsAtItsDefault();
    void aDocumentTheReaderWouldRefuseIsNotWritten_data() const;
    void aDocumentTheReaderWouldRefuseIsNotWritten();
    void anEmptyDirectoryRefusesReadWriteAndErase();
    void eraseTouchesOnlyItsOwnFile();
    void eraseOfAnAbsentFileSucceeds();
    void writeOverAnOldFileReplacesTheWholeContent();
    void anUnknownFieldIsIgnored();
    void anAbsentFieldReadsAtItsDefault();
    void theInMemoryDoubleServesThePortAndCountsItsCalls() const;
    void theInMemoryDoubleCanRefuseLikeItsNeighbour() const;

private:
    [[nodiscard]] QString FilePath() const { return QDir(directory_).filePath(kFileName); }

    QTemporaryDir tempDir_;
    QString directory_;
    int runIndex_ = 0;
    std::optional<LogCapture> capture_;
};

void JsonFileTurnaroundCheckpointStoreTest::initTestCase() const
{
    QVERIFY(tempDir_.isValid());
}

void JsonFileTurnaroundCheckpointStoreTest::init()
{
    directory_ = QDir(tempDir_.path()).filePath(QStringLiteral("run-%1/nested").arg(++runIndex_));
    capture_.emplace();
}

void JsonFileTurnaroundCheckpointStoreTest::cleanup()
{
    capture_.reset();
}

void JsonFileTurnaroundCheckpointStoreTest::theFillHelperChangesEveryRawLeaf() const
{
    QCOMPARE(RawLeavesLeftAtTheirDefault(EveryRawLeafFilled()), 0);
}

void JsonFileTurnaroundCheckpointStoreTest::readOfAnAbsentFileIsNoneAndSilent() const
{
    const JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);

    QVERIFY(!store.Read().has_value());
    QCOMPARE(LogCapture::Count(), 0);
}

void JsonFileTurnaroundCheckpointStoreTest::everyRawLeafRoundTripsThroughARealDirectory()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    const TurnaroundDocument document = FullDocument();

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(*read == document);
    QCOMPARE(LogCapture::Count(), 0);
}

void JsonFileTurnaroundCheckpointStoreTest::aFullDocumentIsFarBelowTheSizeCap()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    const qint64 size = QFileInfo(FilePath()).size();

    QVERIFY(size > 0);
    QVERIFY(size * 16 < kSizeCap);
}

void JsonFileTurnaroundCheckpointStoreTest::writeCreatesTheDirectoryOnFirstUse()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(!QDir(directory_).exists());

    QVERIFY(store.Write(FullDocument()));

    QVERIFY(QFile::exists(FilePath()));
}

void JsonFileTurnaroundCheckpointStoreTest::everyResumablePhaseRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);

    for (int index = 0; index < static_cast<int>(TurnaroundPhase::Count); ++index)
    {
        const auto phase = static_cast<TurnaroundPhase>(index);
        if (!IsResumablePhase(phase))
        {
            continue;
        }
        TurnaroundDocument document = FullDocument();
        document.checkpoint->phase = phase;

        QVERIFY(store.Write(document));
        const auto read = store.Read();

        QVERIFY2(read.has_value() && *read == document, TurnaroundPhaseToString(phase));
    }
}

void JsonFileTurnaroundCheckpointStoreTest::onlyTheLeavesClassedRawAreWritten()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.checkpoint->data = EveryFieldNonDefault();
    QVERIFY(NotRawLeavesDifferingFromTheirDefault(document.checkpoint->data) > 0);

    QVERIFY(store.Write(document));
    const auto read = store.Read();
    const QJsonObject written = QJsonDocument::fromJson(ReadAll(FilePath())).object();
    const QJsonObject writtenData = written.value("checkpoint").toObject().value("data").toObject();
    const std::vector<QString> rawNames = LeafNames(true);

    QVERIFY(!rawNames.empty());
    QCOMPARE(writtenData.size(), static_cast<qsizetype>(rawNames.size()));
    for (const QString& name : rawNames)
    {
        QVERIFY2(writtenData.contains(name), qPrintable(name));
    }
    QCOMPARE(NotRawFilledLeavesFoundIn(writtenData), 0);
    QVERIFY(read.has_value());
    QVERIFY(read->checkpoint.has_value());
    QCOMPARE(NotRawLeavesDifferingFromTheirDefault(read->checkpoint->data), 0);
    QVERIFY(read->checkpoint->data == WithTheNotRawLeavesAtTheirDefault(document.checkpoint->data));
}

void JsonFileTurnaroundCheckpointStoreTest::aLeafClassedNotRawInTheFileReadsAtItsDefault()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    const TurnaroundDocument document = FullDocument();
    QVERIFY(store.Write(document));
    const std::vector<QString> notRawNames = LeafNames(false);
    QVERIFY(!notRawNames.empty());

    QVERIFY(RewriteJson(FilePath(),
                        [&notRawNames](QJsonObject& root)
                        {
                            InData([&notRawNames](QJsonObject& data)
                                   {
                                       for (const QString& name : notRawNames)
                                       {
                                           data.insert(name, 7);
                                       }
                                   })(root);
                        }));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QCOMPARE(NotRawLeavesDifferingFromTheirDefault(read->checkpoint->data), 0);
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aDocumentWithNoCheckpointRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    const TurnaroundDocument document = KeyAndMarkOnly();

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(!read->checkpoint.has_value());
    QVERIFY(!read->plan.has_value());
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aRepositionedMarkOfFalseRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.repositioned = false;

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(!read->repositioned);
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aDefaultDataCheckpointRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.checkpoint->data = TurnaroundData{};

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(read->checkpoint.has_value());
    QVERIFY(read->checkpoint->data == TurnaroundData{});
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aMixOfTrueAndFalseRawBoolsRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.checkpoint->data = WithAlternatingRawBools(document.checkpoint->data);
    QVERIFY(!(document.checkpoint->data == FullDocument().checkpoint->data));
    QVERIFY(!(document.checkpoint->data == TurnaroundData{}));

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aPlanWithoutPayloadOrCargoRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();

    document.plan->payloadKg.reset();
    document.plan->cargoKg.reset();
    QVERIFY(store.Write(document));
    const auto neither = store.Read();
    QVERIFY(neither.has_value());
    QVERIFY(neither->plan.has_value());
    QVERIFY(!neither->plan->payloadKg.has_value());
    QVERIFY(!neither->plan->cargoKg.has_value());
    QVERIFY(*neither == document);

    document.plan->payloadKg = 0.0;
    document.plan->cargoKg.reset();
    QVERIFY(store.Write(document));
    const auto onlyPayload = store.Read();
    QVERIFY(onlyPayload.has_value());
    QVERIFY(onlyPayload->plan->payloadKg.has_value());
    QVERIFY(!onlyPayload->plan->cargoKg.has_value());
    QVERIFY(*onlyPayload == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aPlanInKilogramsRoundTrips()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.plan->unit = WeightUnit::Kg;

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(read->plan->unit == WeightUnit::Kg);
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::accentsBarsQuotesAndNewlinesSurvive()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.key.parkingName = "Terminal 1 | Gate C14";
    document.key.aircraftTitle = "A\xC3\xA9ronef de a\xC3\xA7\xC3\xA3o \xE2\x80\x94 \xC3\xA3";
    document.checkpoint->aircraftMemory.PutText("note", "he said \"go\"\nthen left \\ | \xC3\xA3");
    document.memoryByOwner["gsx"].PutText("line", "a\r\nb\tc");

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QCOMPARE(read->key.parkingName, std::string("Terminal 1 | Gate C14"));
    QCOMPARE(read->key.aircraftTitle, document.key.aircraftTitle);
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aTitleWithAByteOutsideUtf8IsReadBackWithTheReplacementCharacter()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.key.aircraftTitle = "A\xE9"
                                 "ronef";

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(read->key.aircraftTitle != document.key.aircraftTitle);
    QCOMPARE(read->key.aircraftTitle,
             std::string("A\xEF\xBF\xBD"
                         "ronef"));
}

void JsonFileTurnaroundCheckpointStoreTest::doublesExactAndInexactRoundTrip()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    document.plan->fuelKg = 0.1;
    document.plan->zfwKg = 12345.678;
    document.plan->operatingEmptyKg = 1.0 / 3.0;
    document.plan->payloadKg = 0.5;
    document.checkpoint->data.plannedFuelKg = 0.1;
    document.checkpoint->data.plannedZfwKg = 12345.678;
    document.checkpoint->data.settledFuelKg = 2.0 / 3.0;
    document.checkpoint->data.omittedCrewKg = 0.25;

    QVERIFY(store.Write(document));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(read->plan.has_value());
    QVERIFY(read->checkpoint.has_value());
    QCOMPARE(read->plan->fuelKg, 0.1);
    QCOMPARE(read->plan->zfwKg, 12345.678);
    QCOMPARE(read->checkpoint->data.settledFuelKg, 2.0 / 3.0);
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::aNonFiniteRawDoubleReadsAsNoneOnTheNextRead()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);

    for (const double nonFinite : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        TurnaroundDocument document = FullDocument();
        document.checkpoint->data.settledFuelKg = nonFinite;

        QVERIFY(store.Write(document));

        QVERIFY(RefusedBecause(store, "checkpoint is malformed"));
    }
}

void JsonFileTurnaroundCheckpointStoreTest::aTruncatedFileReadsAsNone()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));
    const QByteArray whole = ReadAll(FilePath());
    QVERIFY(whole.size() > 2);

    QVERIFY(WriteAll(FilePath(), whole.left(whole.size() / 2)));

    QVERIFY(RefusedBecause(store, "not a JSON object"));
}

void JsonFileTurnaroundCheckpointStoreTest::anotherClientVersionReadsAsNone()
{
    JsonFileTurnaroundCheckpointStore writer(directory_, kClientVersion);
    QVERIFY(writer.Write(FullDocument()));
    const JsonFileTurnaroundCheckpointStore otherVersion(directory_, QStringLiteral("1.45.0"));

    QVERIFY(RefusedBecause(otherVersion, "client version"));
    QVERIFY(writer.Read().has_value());
}

void JsonFileTurnaroundCheckpointStoreTest::anotherPhaseCountReadsAsNone()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    QVERIFY(RewriteJson(FilePath(), Insert("phaseCount", static_cast<int>(TurnaroundPhase::Count) + 1)));

    QVERIFY(RefusedBecause(store, "phase count"));
}

void JsonFileTurnaroundCheckpointStoreTest::anUnknownPhaseNameReadsAsNone()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    QVERIFY(RewriteJson(FilePath(), Within("checkpoint", Insert("phase", "Taxiing"))));

    QVERIFY(RefusedBecause(store, "checkpoint is malformed"));
}

void JsonFileTurnaroundCheckpointStoreTest::aPhaseOutsideTheResumableRangeReadsAsNone_data() const
{
    QTest::addColumn<QString>("phaseName");

    QTest::newRow("before the reposition") << QStringLiteral("WaitingAircraftReady");
    QTest::newRow("first phase") << QStringLiteral("WaitingSupportedAircraft");
    QTest::newRow("after the cabin services") << QStringLiteral("WaitingNewFlight");
}

void JsonFileTurnaroundCheckpointStoreTest::aPhaseOutsideTheResumableRangeReadsAsNone()
{
    QFETCH(QString, phaseName);
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    QVERIFY(RewriteJson(FilePath(), Within("checkpoint", Insert("phase", phaseName))));

    QVERIFY(RefusedBecause(store, "checkpoint is malformed"));
}

void JsonFileTurnaroundCheckpointStoreTest::jsonOfAnotherShapeReadsAsNone_data() const
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QString>("reason");

    QTest::newRow("an array") << QByteArray("[1, 2, 3]") << QStringLiteral("not a JSON object");
    QTest::newRow("an unrelated object") << QByteArray("{\"hello\": \"world\"}") << QStringLiteral("client version");
    QTest::newRow("a bare number") << QByteArray("42") << QStringLiteral("not a JSON object");
    QTest::newRow("an empty file") << QByteArray() << QStringLiteral("not a JSON object");
    QTest::newRow("not json") << QByteArray("turnaround") << QStringLiteral("not a JSON object");
}

void JsonFileTurnaroundCheckpointStoreTest::jsonOfAnotherShapeReadsAsNone()
{
    QFETCH(QByteArray, content);
    QFETCH(QString, reason);
    const JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(QDir().mkpath(directory_));
    QVERIFY(WriteAll(FilePath(), content));

    QVERIFY2(RefusedBecause(store, qPrintable(reason)), qPrintable(reason));
}

void JsonFileTurnaroundCheckpointStoreTest::aMalformedDocumentReadsAsNone_data() const
{
    QTest::addColumn<Edit>("edit");
    QTest::addColumn<QString>("reason");

    const auto wrongTypes = QJsonValue(QJsonArray{1});
    const auto goodBagItem = QJsonObject{{"name", "a"}, {"value", "b"}};

    AddRefusal("the version is a number", Insert("clientVersion", 1), "client version");
    AddRefusal("the version is absent", Remove("clientVersion"), "client version");
    AddRefusal("the phase count is a string", Insert("phaseCount", "24"), "phase count");
    AddRefusal("the phase count is fractional", Insert("phaseCount", 24.5), "phase count");
    AddRefusal("the phase count is absent", Remove("phaseCount"), "phase count");

    AddRefusal("the key is an array", Insert("key", wrongTypes), "key is malformed");
    AddRefusal("the key is a string", Insert("key", "x"), "key is malformed");
    AddRefusal("the key is absent", Remove("key"), "key is malformed");
    AddRefusal("the key is an empty object", Insert("key", QJsonObject()), "key is malformed");
    for (const char* const part : {"couatlId", "aircraftId", "aircraftTitle", "airportIcao", "parkingName"})
    {
        const QByteArray missing = QByteArray(part) + " is missing";
        const QByteArray mistyped = QByteArray(part) + " is a number";
        AddRefusal(missing.constData(), Within("key", Remove(part)), "key is malformed");
        AddRefusal(mistyped.constData(), Within("key", Insert(part, 5)), "key is malformed");
    }
    AddRefusal("the repositioned mark is a number", Insert("repositioned", 1), "mark is malformed");

    AddRefusal("the checkpoint is a string", Insert("checkpoint", "x"), "checkpoint is malformed");
    AddRefusal("the checkpoint is an array", Insert("checkpoint", wrongTypes), "checkpoint is malformed");
    AddRefusal("the phase is a number", Within("checkpoint", Insert("phase", 5)), "checkpoint is malformed");
    AddRefusal("the phase is absent", Within("checkpoint", Remove("phase")), "checkpoint is malformed");
    AddRefusal("the data is absent", Within("checkpoint", Remove("data")), "checkpoint is malformed");
    AddRefusal("the data is null", Within("checkpoint", Insert("data", QJsonValue::Null)), "checkpoint is malformed");
    AddRefusal("the data is an array", Within("checkpoint", Insert("data", wrongTypes)), "checkpoint is malformed");
    AddRefusal("the data is a string", Within("checkpoint", Insert("data", "x")), "checkpoint is malformed");
    AddRefusal("a raw double is a string", InData(Insert("plannedZfwKg", "x")), "checkpoint is malformed");
    AddRefusal("a raw double is null", InData(Insert("plannedFuelKg", QJsonValue::Null)), "checkpoint is malformed");
    AddRefusal("a raw bool is a number", InData(Insert("gpuRequested", 1)), "checkpoint is malformed");
    AddRefusal("a raw int is fractional", InData(Insert("plannedPassengers", 148.5)), "checkpoint is malformed");
    AddRefusal("a raw int is out of range", InData(Insert("plannedPassengers", 1e30)), "checkpoint is malformed");
    AddRefusal("a raw int overflows int", InData(Insert("plannedPassengers", 3.0e9)), "checkpoint is malformed");
    AddRefusal("a raw int is a string", InData(Insert("plannedPassengers", "148")), "checkpoint is malformed");
    AddRefusal("the aircraft memory is a string", Within("checkpoint", Insert("aircraftMemory", "x")), "checkpoint is malformed");
    AddRefusal("the aircraft memory is an object", Within("checkpoint", Insert("aircraftMemory", QJsonObject())),
               "checkpoint is malformed");
    AddRefusal("an aircraft memory item is a number", Within("checkpoint", Insert("aircraftMemory", BagItems(5))),
               "checkpoint is malformed");
    AddRefusal("an aircraft memory item has no name",
               Within("checkpoint", Insert("aircraftMemory", BagItems(QJsonObject{{"value", "b"}}))), "checkpoint is malformed");
    AddRefusal("an aircraft memory value is a number",
               Within("checkpoint", Insert("aircraftMemory", BagItems(QJsonObject{{"name", "a"}, {"value", 1}}))),
               "checkpoint is malformed");

    AddRefusal("the plan is a string", Insert("plan", "x"), "plan is malformed");
    AddRefusal("the plan is an array", Insert("plan", wrongTypes), "plan is malformed");
    AddRefusal("the plan is empty", Insert("plan", QJsonObject()), "plan is malformed");
    AddRefusal("the plan lacks the fuel", InPlan(Remove("fuelKg")), "plan is malformed");
    AddRefusal("the plan lacks the zero fuel weight", InPlan(Remove("zfwKg")), "plan is malformed");
    AddRefusal("the unit is a string", InPlan(Insert("unit", "lb")), "plan is malformed");
    AddRefusal("the unit is out of range", InPlan(Insert("unit", 5)), "plan is malformed");
    AddRefusal("the unit is fractional", InPlan(Insert("unit", 1.5)), "plan is malformed");
    AddRefusal("the zero fuel weight is a string", InPlan(Insert("zfwKg", "x")), "plan is malformed");
    AddRefusal("the fuel is a string", InPlan(Insert("fuelKg", "x")), "plan is malformed");
    AddRefusal("the passengers are fractional", InPlan(Insert("passengers", 148.5)), "plan is malformed");
    AddRefusal("the origin is a number", InPlan(Insert("origin", 7)), "plan is malformed");
    AddRefusal("the payload is a string", InPlan(Insert("payloadKg", "x")), "plan is malformed");
    AddRefusal("the cargo is a bool", InPlan(Insert("cargoKg", true)), "plan is malformed");

    AddRefusal("the memory is an array", Insert("memoryByOwner", wrongTypes), "memory is malformed");
    AddRefusal("the memory is a string", Insert("memoryByOwner", "x"), "memory is malformed");
    AddRefusal("an owner bag is an object", Within("memoryByOwner", Insert("gsx", QJsonObject())), "memory is malformed");
    AddRefusal("an owner bag item lacks its value",
               Within("memoryByOwner", Insert("gsx", BagItems(QJsonObject{{"name", "a"}}))), "memory is malformed");
    AddRefusal("an owner bag item is bad beside a good one",
               Within("memoryByOwner", Insert("gsx", QJsonArray{goodBagItem, QJsonValue(5)})), "memory is malformed");
}

void JsonFileTurnaroundCheckpointStoreTest::aMalformedDocumentReadsAsNone()
{
    QFETCH(Edit, edit);
    QFETCH(QString, reason);
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    QVERIFY(RewriteJson(FilePath(), edit));

    QVERIFY2(RefusedBecause(store, qPrintable(reason)), qPrintable(reason));
}

void JsonFileTurnaroundCheckpointStoreTest::aFileLargerThanTheCapReadsAsNone()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));
    QVERIFY(store.Read().has_value());

    QVERIFY(RewriteJson(FilePath(), Insert("padding", QString(static_cast<qsizetype>(kSizeCap) + 1, QLatin1Char('x')))));
    QVERIFY(QFileInfo(FilePath()).size() > kSizeCap);

    QVERIFY(RefusedBecause(store, "size cap"));
}

void JsonFileTurnaroundCheckpointStoreTest::aDirectoryAtThePathReadsAsNoneAndCannotBeErased()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(QDir().mkpath(QDir(FilePath()).filePath(QStringLiteral("inner"))));

    QVERIFY(RefusedBecause(store, "cannot be opened"));
    QVERIFY(!store.Erase());
    QVERIFY(!store.Write(FullDocument()));
    QVERIFY(QDir(FilePath()).exists());
}

void JsonFileTurnaroundCheckpointStoreTest::nullContainersReadAsAbsent()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    QVERIFY(store.Write(document));

    QVERIFY(RewriteJson(FilePath(),
                        [](QJsonObject& root)
                        {
                            root.insert("checkpoint", QJsonValue::Null);
                            root.insert("plan", QJsonValue::Null);
                            root.insert("memoryByOwner", QJsonValue::Null);
                        }));
    const auto withoutContainers = store.Read();

    QVERIFY(withoutContainers.has_value());
    QVERIFY(!withoutContainers->checkpoint.has_value());
    QVERIFY(!withoutContainers->plan.has_value());
    QVERIFY(withoutContainers->memoryByOwner.empty());
    QVERIFY(withoutContainers->key == document.key);
    QVERIFY(withoutContainers->repositioned);

    QVERIFY(store.Write(document));
    QVERIFY(RewriteJson(FilePath(),
                        [](QJsonObject& root)
                        {
                            Within("checkpoint", Insert("aircraftMemory", QJsonValue::Null))(root);
                            Within("memoryByOwner", Insert("gsx", QJsonValue::Null))(root);
                        }));
    const auto withoutBags = store.Read();
    document.checkpoint->aircraftMemory = MemoryBag();
    document.memoryByOwner["gsx"] = MemoryBag();

    QVERIFY(withoutBags.has_value());
    QVERIFY(*withoutBags == document);
}

void JsonFileTurnaroundCheckpointStoreTest::anAbsentPlanLeafReadsAtItsDefault()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));

    QVERIFY(RewriteJson(FilePath(),
                        Insert("plan", QJsonObject{{"fuelKg", 100.5}, {"zfwKg", 2000.25}, {"futurePart", true}})));
    const auto read = store.Read();

    FlightPlan expected;
    expected.fuelKg = 100.5;
    expected.zfwKg = 2000.25;

    QVERIFY(read.has_value());
    QVERIFY(read->plan.has_value());
    QVERIFY(*read->plan == expected);
}

void JsonFileTurnaroundCheckpointStoreTest::aDocumentTheReaderWouldRefuseIsNotWritten_data() const
{
    QTest::addColumn<int>("phase");
    QTest::addColumn<int>("unit");

    const int resumable = static_cast<int>(TurnaroundPhase::Loading);
    const int kilograms = static_cast<int>(WeightUnit::Kg);

    QTest::newRow("a phase before the reposition") << static_cast<int>(TurnaroundPhase::WaitingAircraftReady) << kilograms;
    QTest::newRow("a phase after the cabin services") << static_cast<int>(TurnaroundPhase::WaitingNewFlight) << kilograms;
    QTest::newRow("a phase past the last one") << static_cast<int>(TurnaroundPhase::Count) + 3 << kilograms;
    QTest::newRow("a unit past the last one") << resumable << 2;
    QTest::newRow("a negative unit") << resumable << -1;
}

void JsonFileTurnaroundCheckpointStoreTest::aDocumentTheReaderWouldRefuseIsNotWritten()
{
    QFETCH(int, phase);
    QFETCH(int, unit);
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    const TurnaroundDocument kept = FullDocument();
    QVERIFY(store.Write(kept));
    const QByteArray before = ReadAll(FilePath());
    TurnaroundDocument refused = FullDocument();
    refused.checkpoint->phase = static_cast<TurnaroundPhase>(phase);
    refused.plan->unit = static_cast<WeightUnit>(unit);

    QVERIFY(!store.Write(refused));

    QCOMPARE(ReadAll(FilePath()), before);
    QVERIFY(*store.Read() == kept);
    QCOMPARE(QDir(directory_).entryList(QDir::NoDotAndDotDot | QDir::AllEntries), QStringList{QString(kFileName)});
}

void JsonFileTurnaroundCheckpointStoreTest::anEmptyDirectoryRefusesReadWriteAndErase()
{
    QTemporaryDir workingDirectory;
    QVERIFY(workingDirectory.isValid());
    const QString previous = QDir::currentPath();
    QVERIFY(QDir::setCurrent(workingDirectory.path()));
    const auto restoreWorkingDirectory = qScopeGuard([&previous] { QDir::setCurrent(previous); });
    JsonFileTurnaroundCheckpointStore real(workingDirectory.path(), kClientVersion);
    QVERIFY(real.Write(FullDocument()));
    QVERIFY(QFile::exists(QDir::current().filePath(kFileName)));
    JsonFileTurnaroundCheckpointStore unconfigured(QString(), kClientVersion);

    QVERIFY(!unconfigured.Read().has_value());
    QVERIFY(!unconfigured.Write(KeyAndMarkOnly()));
    QVERIFY(!unconfigured.Erase());

    QVERIFY(real.Read().has_value());
    QVERIFY(*real.Read() == FullDocument());
}

void JsonFileTurnaroundCheckpointStoreTest::eraseTouchesOnlyItsOwnFile()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));
    const QString neighbour = QDir(directory_).filePath(QStringLiteral("neighbour.txt"));
    const QString lookalike = QDir(directory_).filePath(QStringLiteral("turnaround.json.bak"));
    QVERIFY(WriteAll(neighbour, "keep me"));
    QVERIFY(WriteAll(lookalike, "keep me too"));

    QVERIFY(store.Erase());

    QVERIFY(!QFile::exists(FilePath()));
    QVERIFY(!store.Read().has_value());
    QCOMPARE(ReadAll(neighbour), QByteArray("keep me"));
    QCOMPARE(ReadAll(lookalike), QByteArray("keep me too"));
    QVERIFY(QDir(directory_).exists());
}

void JsonFileTurnaroundCheckpointStoreTest::eraseOfAnAbsentFileSucceeds()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);

    QVERIFY(store.Erase());
}

void JsonFileTurnaroundCheckpointStoreTest::writeOverAnOldFileReplacesTheWholeContent()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    QVERIFY(store.Write(FullDocument()));
    const TurnaroundDocument replacement = KeyAndMarkOnly();

    QVERIFY(store.Write(replacement));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(*read == replacement);
    QCOMPARE(QDir(directory_).entryList(QDir::NoDotAndDotDot | QDir::AllEntries), QStringList{QString(kFileName)});
}

void JsonFileTurnaroundCheckpointStoreTest::anUnknownFieldIsIgnored()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    const TurnaroundDocument document = FullDocument();
    QVERIFY(store.Write(document));

    QVERIFY(RewriteJson(FilePath(),
                        [](QJsonObject& root)
                        {
                            Insert("futureTopLevel", "x")(root);
                            InData(Insert("futureLeaf", 7))(root);
                            Within("checkpoint", Insert("futurePart", true))(root);
                            Within("plan", Insert("futurePlanPart", "y"))(root);
                            Within("key", Insert("futureKeyPart", 2))(root);
                        }));
    const auto read = store.Read();

    QVERIFY(read.has_value());
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::anAbsentFieldReadsAtItsDefault()
{
    JsonFileTurnaroundCheckpointStore store(directory_, kClientVersion);
    TurnaroundDocument document = FullDocument();
    QVERIFY(store.Write(document));

    QVERIFY(RewriteJson(FilePath(),
                        [](QJsonObject& root)
                        {
                            InData(
                                [](QJsonObject& data)
                                {
                                    data.remove("plannedFuelKg");
                                    data.remove("lavatory.asked");
                                })(root);
                            Remove("repositioned")(root);
                        }));
    const auto read = store.Read();
    document.checkpoint->data.plannedFuelKg = 0.0;
    document.checkpoint->data.lavatory.asked = false;
    document.repositioned = false;

    QVERIFY(read.has_value());
    QVERIFY(*read == document);
}

void JsonFileTurnaroundCheckpointStoreTest::theInMemoryDoubleServesThePortAndCountsItsCalls() const
{
    FakeTurnaroundCheckpointStore fake;
    TurnaroundCheckpointStore& port = fake;
    QVERIFY(!port.Read().has_value());

    QVERIFY(port.Write(FullDocument()));
    const auto read = port.Read();
    QVERIFY(port.Erase());

    QVERIFY(read.has_value());
    QVERIFY(*read == FullDocument());
    QVERIFY(!fake.stored.has_value());
    QCOMPARE(fake.readCalls, 2);
    QCOMPARE(fake.writeCalls, 1);
    QCOMPARE(fake.eraseCalls, 1);
}

void JsonFileTurnaroundCheckpointStoreTest::theInMemoryDoubleCanRefuseLikeItsNeighbour() const
{
    FakeTurnaroundCheckpointStore fake;
    TurnaroundCheckpointStore& port = fake;
    QVERIFY(port.Write(KeyAndMarkOnly()));

    fake.writeResult = false;
    QVERIFY(!port.Write(FullDocument()));
    QVERIFY(fake.stored.has_value());
    QVERIFY(*fake.stored == KeyAndMarkOnly());

    fake.eraseResult = false;
    QVERIFY(!port.Erase());
    QVERIFY(fake.stored.has_value());

    fake.eraseResult = true;
    QVERIFY(port.Erase());
    QVERIFY(!fake.stored.has_value());
    QCOMPARE(fake.writeCalls, 2);
    QCOMPARE(fake.eraseCalls, 2);
}

QTEST_GUILESS_MAIN(JsonFileTurnaroundCheckpointStoreTest)

#include "tst_json_file_turnaround_checkpoint_store.moc"
