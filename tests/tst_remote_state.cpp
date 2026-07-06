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
    static void messagePatchTogglesVisibility();
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
    QCOMPARE(gpu->state, std::string{"performing"});
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

void RemoteStateTest::messagePatchTogglesVisibility()
{
    GsxRemoteState state;
    const QJsonObject message = QJsonDocument::fromJson(
        R"({"text":"Fuel Truck is on its way","visible":true})").object();

    GsxRemoteStateReducer::ApplyPatch(state, "/message", message);

    QVERIFY(state.message.visible);
    QCOMPARE(state.message.text, std::string{"Fuel Truck is on its way"});
}

QTEST_APPLESS_MAIN(RemoteStateTest)

#include "tst_remote_state.moc"
