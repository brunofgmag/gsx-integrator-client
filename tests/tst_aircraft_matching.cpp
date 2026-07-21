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
    static void sharedIcaoIsDisambiguatedByTitleRule();
    static void titleOnlyRuleIgnoresBareIcao();
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

void AircraftMatchingTest::sharedIcaoIsDisambiguatedByTitleRule()
{
    const AircraftDescriptor freighter{
        "PMDG 777F",
        {{MatchField::Title, MatchOp::StartsWith, "777F"}},
        &NullCreator};
    const AircraftDescriptor longRange{
        "PMDG 777-200LR",
        {{MatchField::Title, MatchOp::StartsWith, "777-200LR"}},
        &NullCreator};

    const AircraftIdentity freighterIdentity{"777F", "B77L"};
    const AircraftIdentity longRangeIdentity{"777-200LR", "B77L"};

    const AircraftDescriptor* freighterWinner =
        MatchAircraft({&freighter, &longRange}, freighterIdentity);
    const AircraftDescriptor* longRangeWinner =
        MatchAircraft({&freighter, &longRange}, longRangeIdentity);

    QVERIFY(freighterWinner != nullptr);
    QVERIFY(longRangeWinner != nullptr);
    QCOMPARE(freighterWinner->name, "PMDG 777F");
    QCOMPARE(longRangeWinner->name, "PMDG 777-200LR");
}

void AircraftMatchingTest::titleOnlyRuleIgnoresBareIcao()
{
    const AircraftDescriptor titleOnly{
        "PMDG 777-300ER",
        {{MatchField::Title, MatchOp::StartsWith, "777-300ER"}},
        &NullCreator};
    const AircraftIdentity bareIcao{"Generic Boeing Repaint", "B77W"};

    QVERIFY(MatchAircraft({&titleOnly}, bareIcao) == nullptr);
}

QTEST_APPLESS_MAIN(AircraftMatchingTest)

#include "tst_aircraft_matching.moc"
