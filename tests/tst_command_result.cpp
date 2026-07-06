#include <QtTest/QTest>

#include "../src/application/model/CommandResult.h"

class CommandResultTest final : public QObject
{
    Q_OBJECT

private slots:
    static void successHasNoMessage();
    static void failureCarriesMessage();
    static void defaultIsFailureWithEmptyMessage();
};

void CommandResultTest::successHasNoMessage()
{
    const auto [succeeded, message] = CommandResult::Success();

    QVERIFY(succeeded);
    QVERIFY(message.empty());
}

void CommandResultTest::failureCarriesMessage()
{
    const auto [succeeded, message] = CommandResult::Failure("denied");

    QVERIFY(!succeeded);
    QCOMPARE(message, std::string("denied"));
}

void CommandResultTest::defaultIsFailureWithEmptyMessage()
{
    const CommandResult result;

    QVERIFY(!result.succeeded);
    QVERIFY(result.message.empty());
}

QTEST_APPLESS_MAIN(CommandResultTest)

#include "tst_command_result.moc"
