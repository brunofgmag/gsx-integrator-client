#include "JsonFileTurnaroundCheckpointStore.h"

#include <concepts>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kFileName = QLatin1StringView("turnaround.json");
    constexpr auto kClientVersionKey = QLatin1StringView("clientVersion");
    constexpr auto kPhaseCountKey = QLatin1StringView("phaseCount");
    constexpr auto kKeyKey = QLatin1StringView("key");
    constexpr auto kCouatlIdKey = QLatin1StringView("couatlId");
    constexpr auto kAircraftIdKey = QLatin1StringView("aircraftId");
    constexpr auto kAircraftTitleKey = QLatin1StringView("aircraftTitle");
    constexpr auto kAirportIcaoKey = QLatin1StringView("airportIcao");
    constexpr auto kParkingNameKey = QLatin1StringView("parkingName");
    constexpr auto kRepositionedKey = QLatin1StringView("repositioned");
    constexpr auto kCheckpointKey = QLatin1StringView("checkpoint");
    constexpr auto kPhaseKey = QLatin1StringView("phase");
    constexpr auto kDataKey = QLatin1StringView("data");
    constexpr auto kAircraftMemoryKey = QLatin1StringView("aircraftMemory");
    constexpr auto kPlanKey = QLatin1StringView("plan");
    constexpr auto kFuelKgKey = QLatin1StringView("fuelKg");
    constexpr auto kZfwKgKey = QLatin1StringView("zfwKg");
    constexpr auto kPassengersKey = QLatin1StringView("passengers");
    constexpr auto kUnitKey = QLatin1StringView("unit");
    constexpr auto kOriginKey = QLatin1StringView("origin");
    constexpr auto kDestinationKey = QLatin1StringView("destination");
    constexpr auto kGeneratedEpochKey = QLatin1StringView("generatedEpoch");
    constexpr auto kOperatingEmptyKgKey = QLatin1StringView("operatingEmptyKg");
    constexpr auto kPayloadKgKey = QLatin1StringView("payloadKg");
    constexpr auto kCargoKgKey = QLatin1StringView("cargoKg");
    constexpr auto kMemoryByOwnerKey = QLatin1StringView("memoryByOwner");
    constexpr auto kEntryNameKey = QLatin1StringView("name");
    constexpr auto kEntryValueKey = QLatin1StringView("value");

    constexpr auto kRefusedUnreadable = "the file exists but cannot be opened";
    constexpr auto kRefusedTooLarge = "the file is larger than the size cap";
    constexpr auto kRefusedNotAnObject = "the content is not a JSON object";
    constexpr auto kRefusedVersion = "the stamp does not carry this client version";
    constexpr auto kRefusedPhaseCount = "the stamp does not carry this phase count";
    constexpr auto kRefusedKey = "the key is malformed";
    constexpr auto kRefusedCheckpoint = "the checkpoint is malformed";
    constexpr auto kRefusedPlan = "the plan is malformed";
    constexpr auto kRefusedMemory = "the memory is malformed";
    constexpr auto kRefusedMark = "the repositioned mark is malformed";

    constexpr int kPhaseCount = static_cast<int>(TurnaroundPhase::Count);
    constexpr qint64 kMaxFileBytes = 1024 * 1024;

    template <typename Value>
    struct IsOptional : std::false_type
    {
    };

    template <typename Inner>
    struct IsOptional<std::optional<Inner>> : std::true_type
    {
    };

    template <typename Value>
    concept ScalarCodec = std::same_as<Value, bool> || std::is_enum_v<Value> || std::integral<Value> ||
                          std::floating_point<Value> || std::same_as<Value, std::string>;

    template <typename Value>
    struct CodecShape : std::bool_constant<ScalarCodec<Value>>
    {
    };

    template <typename Inner>
    struct CodecShape<std::optional<Inner>> : CodecShape<Inner>
    {
    };

    template <typename Value>
    concept HasCodec = CodecShape<Value>::value;

    constexpr bool EveryRawLeafHasACodec()
    {
        bool every = true;
        turnaround::VisitFields(
            [&every](const char*, [[maybe_unused]] const auto accessor, const turnaround::FieldRestore restore)
            {
                using Leaf = std::remove_cvref_t<std::invoke_result_t<decltype(accessor)&, TurnaroundData&>>;
                if (restore == turnaround::FieldRestore::Raw && !HasCodec<Leaf>)
                {
                    every = false;
                }
            });

        return every;
    }

    static_assert(EveryRawLeafHasACodec(), "a TurnaroundData leaf classed Raw has no JSON codec");

    template <typename Value>
    QJsonValue Encode(const Value& value)
    {
        if constexpr (std::same_as<Value, bool>)
        {
            return QJsonValue(value);
        }
        else if constexpr (std::is_enum_v<Value> || std::integral<Value>)
        {
            return QJsonValue(static_cast<qint64>(value));
        }
        else if constexpr (std::floating_point<Value>)
        {
            return QJsonValue(static_cast<double>(value));
        }
        else if constexpr (std::same_as<Value, std::string>)
        {
            return QJsonValue(QString::fromStdString(value));
        }
        else if constexpr (IsOptional<Value>::value)
        {
            return value ? Encode(*value) : QJsonValue(QJsonValue::Null);
        }
        else
        {
            static_assert(sizeof(Value) == 0, "teach Encode the type of the new leaf");
        }
    }

    template <typename Stored, typename Value>
    bool DecodeWhole(const QJsonValue& json, Value& target)
    {
        if (!json.isDouble())
        {
            return false;
        }
        const qint64 whole = json.toInteger();
        if (static_cast<double>(whole) != json.toDouble() || !std::in_range<Stored>(whole))
        {
            return false;
        }
        target = static_cast<Value>(whole);

        return true;
    }

    template <typename Value>
    [[nodiscard]] bool Decode(const QJsonValue& json, Value& target)
    {
        if constexpr (std::same_as<Value, bool>)
        {
            if (!json.isBool())
            {
                return false;
            }
            target = json.toBool();

            return true;
        }
        else if constexpr (std::is_enum_v<Value>)
        {
            return DecodeWhole<std::underlying_type_t<Value>>(json, target);
        }
        else if constexpr (std::integral<Value>)
        {
            return DecodeWhole<Value>(json, target);
        }
        else if constexpr (std::floating_point<Value>)
        {
            if (!json.isDouble())
            {
                return false;
            }
            target = static_cast<Value>(json.toDouble());

            return true;
        }
        else if constexpr (std::same_as<Value, std::string>)
        {
            if (!json.isString())
            {
                return false;
            }
            target = json.toString().toStdString();

            return true;
        }
        else if constexpr (IsOptional<Value>::value)
        {
            if (json.isNull())
            {
                target.reset();

                return true;
            }
            typename Value::value_type inner{};
            if (!Decode(json, inner))
            {
                return false;
            }
            target = std::move(inner);

            return true;
        }
        else
        {
            static_assert(sizeof(Value) == 0, "teach Decode the type of the new leaf");
        }
    }

    template <typename Value>
    [[nodiscard]] bool DecodeIfPresent(const QJsonValue& json, Value& target)
    {
        return json.isUndefined() || Decode(json, target);
    }

    bool IsAbsent(const QJsonValue& json)
    {
        return json.isUndefined() || json.isNull();
    }

    template <typename Owner, typename Visitor>
    void VisitRawLeaves(Owner& data, Visitor&& visit)
    {
        turnaround::VisitFields(
            [&data, &visit](const char* const name, const auto accessor, const turnaround::FieldRestore restore)
            {
                using Leaf = std::remove_cvref_t<decltype(accessor(data))>;
                if constexpr (HasCodec<Leaf>)
                {
                    if (restore == turnaround::FieldRestore::Raw)
                    {
                        visit(QLatin1StringView(name), accessor(data));
                    }
                }
            });
    }

    QJsonObject DataToJson(const TurnaroundData& data)
    {
        QJsonObject object;
        VisitRawLeaves(data, [&object](const QLatin1StringView name, const auto& leaf) { object.insert(name, Encode(leaf)); });

        return object;
    }

    bool DataFromJson(const QJsonObject& object, TurnaroundData& data)
    {
        bool restored = true;
        VisitRawLeaves(data,
                       [&object, &restored](const QLatin1StringView name, auto& leaf)
                       {
                           if (!DecodeIfPresent(object.value(name), leaf))
                           {
                               restored = false;
                           }
                       });

        return restored;
    }

    QJsonArray BagToJson(const MemoryBag& bag)
    {
        QJsonArray entries;
        for (const auto& [name, value] : bag.Entries())
        {
            QJsonObject entry;
            entry.insert(kEntryNameKey, Encode(name));
            entry.insert(kEntryValueKey, Encode(value));
            entries.append(entry);
        }

        return entries;
    }

    bool BagFromJson(const QJsonValue& json, MemoryBag& bag)
    {
        if (IsAbsent(json))
        {
            return true;
        }
        if (!json.isArray())
        {
            return false;
        }

        const QJsonArray items = json.toArray();
        std::vector<MemoryBag::Entry> entries;
        for (const QJsonValue item : items)
        {
            const QJsonObject entry = item.toObject();
            MemoryBag::Entry parsed;
            if (!Decode(entry.value(kEntryNameKey), parsed.first) || !Decode(entry.value(kEntryValueKey), parsed.second))
            {
                return false;
            }
            entries.push_back(std::move(parsed));
        }
        bag = MemoryBag(std::move(entries));

        return true;
    }

    QJsonObject KeyToJson(const TurnaroundKey& key)
    {
        const auto& [couatlId, aircraftId, aircraftTitle, airportIcao, parkingName] = key;
        QJsonObject object;
        object.insert(kCouatlIdKey, Encode(couatlId));
        object.insert(kAircraftIdKey, Encode(aircraftId));
        object.insert(kAircraftTitleKey, Encode(aircraftTitle));
        object.insert(kAirportIcaoKey, Encode(airportIcao));
        object.insert(kParkingNameKey, Encode(parkingName));

        return object;
    }

    bool KeyFromJson(const QJsonValue& json, TurnaroundKey& key)
    {
        auto& [couatlId, aircraftId, aircraftTitle, airportIcao, parkingName] = key;
        const QJsonObject object = json.toObject();

        return Decode(object.value(kCouatlIdKey), couatlId) && Decode(object.value(kAircraftIdKey), aircraftId) &&
               Decode(object.value(kAircraftTitleKey), aircraftTitle) && Decode(object.value(kAirportIcaoKey), airportIcao) &&
               Decode(object.value(kParkingNameKey), parkingName);
    }

    QJsonObject CheckpointToJson(const TurnaroundCheckpoint& checkpoint)
    {
        const auto& [phase, data, aircraftMemory] = checkpoint;
        QJsonObject object;
        object.insert(kPhaseKey, QString::fromLatin1(TurnaroundPhaseToString(phase)));
        object.insert(kDataKey, DataToJson(data));
        object.insert(kAircraftMemoryKey, BagToJson(aircraftMemory));

        return object;
    }

    std::optional<TurnaroundPhase> ResumablePhaseFrom(const QJsonValue& json)
    {
        std::string name;
        if (!Decode(json, name))
        {
            return std::nullopt;
        }
        const auto phase = TurnaroundPhaseFromString(name);

        return phase && IsResumablePhase(*phase) ? phase : std::nullopt;
    }

    bool CheckpointFromJson(const QJsonValue& json, std::optional<TurnaroundCheckpoint>& checkpoint)
    {
        if (IsAbsent(json))
        {
            return true;
        }
        if (!json.isObject())
        {
            return false;
        }

        const QJsonObject object = json.toObject();
        const auto resumable = ResumablePhaseFrom(object.value(kPhaseKey));
        const QJsonValue dataJson = object.value(kDataKey);
        if (!resumable || !dataJson.isObject())
        {
            return false;
        }

        TurnaroundCheckpoint parsed;
        auto& [phase, data, aircraftMemory] = parsed;
        phase = *resumable;
        if (!DataFromJson(dataJson.toObject(), data) || !BagFromJson(object.value(kAircraftMemoryKey), aircraftMemory))
        {
            return false;
        }
        checkpoint = std::move(parsed);

        return true;
    }

    QJsonObject PlanToJson(const FlightPlan& plan)
    {
        const auto& [fuelKg, zfwKg, passengers, unit, origin, destination, generatedEpoch, operatingEmptyKg, payloadKg, cargoKg] = plan;
        QJsonObject object;
        object.insert(kFuelKgKey, Encode(fuelKg));
        object.insert(kZfwKgKey, Encode(zfwKg));
        object.insert(kPassengersKey, Encode(passengers));
        object.insert(kUnitKey, Encode(unit));
        object.insert(kOriginKey, Encode(origin));
        object.insert(kDestinationKey, Encode(destination));
        object.insert(kGeneratedEpochKey, Encode(generatedEpoch));
        object.insert(kOperatingEmptyKgKey, Encode(operatingEmptyKg));
        object.insert(kPayloadKgKey, Encode(payloadKg));
        object.insert(kCargoKgKey, Encode(cargoKg));

        return object;
    }

    bool IsKnownUnit(const WeightUnit unit)
    {
        return unit == WeightUnit::Kg || unit == WeightUnit::Lb;
    }

    bool PlanFromJson(const QJsonValue& json, std::optional<FlightPlan>& plan)
    {
        if (IsAbsent(json))
        {
            return true;
        }
        if (!json.isObject())
        {
            return false;
        }

        const QJsonObject object = json.toObject();
        FlightPlan parsed;
        auto& [fuelKg, zfwKg, passengers, unit, origin, destination, generatedEpoch, operatingEmptyKg, payloadKg, cargoKg] = parsed;
        const bool restored =
            Decode(object.value(kFuelKgKey), fuelKg) && Decode(object.value(kZfwKgKey), zfwKg) &&
            DecodeIfPresent(object.value(kPassengersKey), passengers) && DecodeIfPresent(object.value(kUnitKey), unit) &&
            DecodeIfPresent(object.value(kOriginKey), origin) && DecodeIfPresent(object.value(kDestinationKey), destination) &&
            DecodeIfPresent(object.value(kGeneratedEpochKey), generatedEpoch) &&
            DecodeIfPresent(object.value(kOperatingEmptyKgKey), operatingEmptyKg) &&
            DecodeIfPresent(object.value(kPayloadKgKey), payloadKg) && DecodeIfPresent(object.value(kCargoKgKey), cargoKg);
        if (!restored || !IsKnownUnit(unit))
        {
            return false;
        }
        plan = std::move(parsed);

        return true;
    }

    QJsonObject MemoryToJson(const std::map<std::string, MemoryBag>& memoryByOwner)
    {
        QJsonObject object;
        for (const auto& [owner, bag] : memoryByOwner)
        {
            object.insert(QString::fromStdString(owner), BagToJson(bag));
        }

        return object;
    }

    bool MemoryFromJson(const QJsonValue& json, std::map<std::string, MemoryBag>& memoryByOwner)
    {
        if (IsAbsent(json))
        {
            return true;
        }
        if (!json.isObject())
        {
            return false;
        }

        const QJsonObject object = json.toObject();
        for (const QString& owner : object.keys())
        {
            if (!BagFromJson(object.value(owner), memoryByOwner[owner.toStdString()]))
            {
                return false;
            }
        }

        return true;
    }

    QJsonObject DocumentToJson(const TurnaroundDocument& document, const QString& clientVersion)
    {
        const auto& [key, checkpoint, plan, memoryByOwner, repositioned] = document;
        QJsonObject object;
        object.insert(kClientVersionKey, clientVersion);
        object.insert(kPhaseCountKey, kPhaseCount);
        object.insert(kKeyKey, KeyToJson(key));
        object.insert(kRepositionedKey, repositioned);
        object.insert(kMemoryByOwnerKey, MemoryToJson(memoryByOwner));
        if (checkpoint)
        {
            object.insert(kCheckpointKey, CheckpointToJson(*checkpoint));
        }
        if (plan)
        {
            object.insert(kPlanKey, PlanToJson(*plan));
        }

        return object;
    }

    bool CarriesCurrentPhaseCount(const QJsonValue& json)
    {
        int phaseCount = 0;

        return Decode(json, phaseCount) && phaseCount == kPhaseCount;
    }

    const char* RefusalOf(const QJsonObject& object, const QString& clientVersion, TurnaroundDocument& document)
    {
        const QJsonValue version = object.value(kClientVersionKey);
        if (!version.isString() || version.toString() != clientVersion)
        {
            return kRefusedVersion;
        }
        if (!CarriesCurrentPhaseCount(object.value(kPhaseCountKey)))
        {
            return kRefusedPhaseCount;
        }

        auto& [key, checkpoint, plan, memoryByOwner, repositioned] = document;
        if (!KeyFromJson(object.value(kKeyKey), key))
        {
            return kRefusedKey;
        }
        if (!CheckpointFromJson(object.value(kCheckpointKey), checkpoint))
        {
            return kRefusedCheckpoint;
        }
        if (!PlanFromJson(object.value(kPlanKey), plan))
        {
            return kRefusedPlan;
        }
        if (!MemoryFromJson(object.value(kMemoryByOwnerKey), memoryByOwner))
        {
            return kRefusedMemory;
        }
        if (!DecodeIfPresent(object.value(kRepositionedKey), repositioned))
        {
            return kRefusedMark;
        }

        return nullptr;
    }

    const char* RefusalOfFile(const QString& path, const QString& clientVersion, TurnaroundDocument& document)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return kRefusedUnreadable;
        }
        if (file.size() > kMaxFileBytes)
        {
            return kRefusedTooLarge;
        }

        const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
        if (!json.isObject())
        {
            return kRefusedNotAnObject;
        }

        return RefusalOf(json.object(), clientVersion, document);
    }

    bool CanBeReadBack(const TurnaroundDocument& document)
    {
        return (!document.checkpoint || IsResumablePhase(document.checkpoint->phase)) &&
               (!document.plan || IsKnownUnit(document.plan->unit));
    }
}

JsonFileTurnaroundCheckpointStore::JsonFileTurnaroundCheckpointStore(QString directory, QString clientVersion)
    : directory_(std::move(directory))
    , clientVersion_(std::move(clientVersion))
{
}

QString JsonFileTurnaroundCheckpointStore::FilePath() const
{
    return directory_.isEmpty() ? QString() : QDir(directory_).filePath(QString(kFileName));
}

std::optional<TurnaroundDocument> JsonFileTurnaroundCheckpointStore::Read() const
{
    const QString path = FilePath();
    if (path.isEmpty() || !QFile::exists(path))
    {
        return std::nullopt;
    }

    TurnaroundDocument document;
    if (const char* const refusal = RefusalOfFile(path, clientVersion_, document))
    {
        LOG_WARN("Turnaround file %s ignored: %s", qUtf8Printable(path), refusal);

        return std::nullopt;
    }

    return document;
}

bool JsonFileTurnaroundCheckpointStore::Write(const TurnaroundDocument& document)
{
    const QString path = FilePath();
    if (path.isEmpty() || !CanBeReadBack(document) || !QDir().mkpath(directory_))
    {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    const QByteArray bytes = QJsonDocument(DocumentToJson(document, clientVersion_)).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size())
    {
        file.cancelWriting();

        return false;
    }

    return file.commit();
}

bool JsonFileTurnaroundCheckpointStore::Erase()
{
    const QString path = FilePath();

    return !path.isEmpty() && (!QFile::exists(path) || QFile::remove(path));
}
