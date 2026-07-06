#include <QtTest/QTest>

#include <memory>
#include <vector>
#include "../src/infrastructure/aircraft/AircraftRegistry.h"

namespace
{
    std::unique_ptr<Aircraft> NullCreator(VariableGateway*, AutomationStatus*, const AircraftIdentity&)
    {
        return nullptr;
    }
}

class AircraftMatchingTest final : public QObject
{
    Q_OBJECT

private slots:
    static void equalsIsCaseInsensitive();
    static void startsWithMatchesPrefixOnly();
    static void containsMatchesSubstring();
    static void emptyPatternNeverMatches();
    static void returnsNullWithoutCandidates();
    static void returnsNullWhenNothingMatches();
    static void sameFieldScoresOnce();
    static void atcModelOutweighsTitle();
    static void bothFieldsBeatSingleField();
    static void tieBreaksByNameRegardlessOfOrder();
};

void AircraftMatchingTest::equalsIsCaseInsensitive()
{
    QVERIFY(MatchText("MD11", MatchOp::Equals, "md11"));
    QVERIFY(!MatchText("MD11F", MatchOp::Equals, "md11"));
}

void AircraftMatchingTest::startsWithMatchesPrefixOnly()
{
    QVERIFY(MatchText("TFDi Design MD-11 PW44", MatchOp::StartsWith, "tfdi design"));
    QVERIFY(!MatchText("Livery TFDi Design MD-11", MatchOp::StartsWith, "tfdi design"));
}

void AircraftMatchingTest::containsMatchesSubstring()
{
    QVERIFY(MatchText("FedEx md-11f N525FE", MatchOp::Contains, "MD-11F"));
    QVERIFY(!MatchText("Airbus A320", MatchOp::Contains, "MD-11"));
}

void AircraftMatchingTest::emptyPatternNeverMatches()
{
    QVERIFY(!MatchText("TFDi Design MD-11", MatchOp::Equals, ""));
    QVERIFY(!MatchText("TFDi Design MD-11", MatchOp::StartsWith, ""));
    QVERIFY(!MatchText("TFDi Design MD-11", MatchOp::Contains, ""));
}

void AircraftMatchingTest::returnsNullWithoutCandidates()
{
    const AircraftIdentity identity{"TFDi Design MD-11", "MD11"};

    QVERIFY(MatchAircraft({}, identity) == nullptr);
}

void AircraftMatchingTest::returnsNullWhenNothingMatches()
{
    const AircraftDescriptor descriptor{
        "Alpha",
        {{MatchField::Title, MatchOp::Contains, "MD-11"}},
        &NullCreator};
    const AircraftIdentity identity{"Airbus A320", "A320"};

    QVERIFY(MatchAircraft({&descriptor}, identity) == nullptr);
}

void AircraftMatchingTest::sameFieldScoresOnce()
{
    const AircraftDescriptor doubleRules{
        "Zulu",
        {{MatchField::Title, MatchOp::Contains, "MD-11"},
         {MatchField::Title, MatchOp::Contains, "TFDi"}},
        &NullCreator};
    const AircraftDescriptor singleRule{
        "Alpha",
        {{MatchField::Title, MatchOp::Contains, "Design"}},
        &NullCreator};
    const AircraftIdentity identity{"TFDi Design MD-11", ""};

    const AircraftDescriptor* winner = MatchAircraft({&doubleRules, &singleRule}, identity);

    QVERIFY(winner != nullptr);
    QCOMPARE(winner->name, "Alpha");
}

void AircraftMatchingTest::atcModelOutweighsTitle()
{
    const AircraftDescriptor byTitle{
        "TitleBird",
        {{MatchField::Title, MatchOp::Contains, "MD-11"}},
        &NullCreator};
    const AircraftDescriptor byAtcModel{
        "ModelBird",
        {{MatchField::AtcModel, MatchOp::Equals, "MD11"}},
        &NullCreator};
    const AircraftIdentity identity{"TFDi Design MD-11", "MD11"};

    const AircraftDescriptor* winner = MatchAircraft({&byTitle, &byAtcModel}, identity);

    QVERIFY(winner != nullptr);
    QCOMPARE(winner->name, "ModelBird");
}

void AircraftMatchingTest::bothFieldsBeatSingleField()
{
    const AircraftDescriptor bothFields{
        "Zulu",
        {{MatchField::Title, MatchOp::Contains, "MD-11"},
         {MatchField::AtcModel, MatchOp::Equals, "MD11"}},
        &NullCreator};
    const AircraftDescriptor atcModelOnly{
        "Alpha",
        {{MatchField::AtcModel, MatchOp::Equals, "MD11"}},
        &NullCreator};
    const AircraftIdentity identity{"TFDi Design MD-11", "MD11"};

    const AircraftDescriptor* winner = MatchAircraft({&atcModelOnly, &bothFields}, identity);

    QVERIFY(winner != nullptr);
    QCOMPARE(winner->name, "Zulu");
}

void AircraftMatchingTest::tieBreaksByNameRegardlessOfOrder()
{
    const AircraftDescriptor bravo{
        "Bravo",
        {{MatchField::Title, MatchOp::Contains, "MD-11"}},
        &NullCreator};
    const AircraftDescriptor alpha{
        "Alpha",
        {{MatchField::Title, MatchOp::Contains, "TFDi"}},
        &NullCreator};
    const AircraftIdentity identity{"TFDi Design MD-11", ""};

    const AircraftDescriptor* firstOrder = MatchAircraft({&bravo, &alpha}, identity);
    const AircraftDescriptor* secondOrder = MatchAircraft({&alpha, &bravo}, identity);

    QVERIFY(firstOrder != nullptr);
    QVERIFY(secondOrder != nullptr);
    QCOMPARE(firstOrder->name, "Alpha");
    QCOMPARE(secondOrder->name, "Alpha");
}

QTEST_APPLESS_MAIN(AircraftMatchingTest)

#include "tst_aircraft_matching.moc"
