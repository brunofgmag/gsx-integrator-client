#include <optional>

#include <QtTest/QTest>

#include "../src/application/TurnaroundKeyJudgement.h"

Q_DECLARE_METATYPE(TurnaroundPhase)

namespace
{
    constexpr auto kSavedCouatl = "2120420471";
    constexpr auto kLiveCouatl = "2123650379";
    constexpr auto kAircraftId = "fss-727-200f";
    constexpr auto kOtherAircraftId = "tfdi-md11";
    constexpr auto kTitle = "FSS Boeing 727-200F";
    constexpr auto kOtherTitle = "FSS Boeing 727-200F Cargo";
    constexpr auto kAirport = "CDK2";
    constexpr auto kOtherAirport = "CYEG";
    constexpr auto kParking = "Parking 2";
    constexpr auto kOtherParking = "Parking 5";

    TurnaroundKey SavedKey()
    {
        return TurnaroundKey{kSavedCouatl, kAircraftId, kTitle, kAirport, kParking};
    }

    TurnaroundKey LiveKey()
    {
        return TurnaroundKey{kSavedCouatl, kAircraftId, kTitle, kAirport, kParking};
    }

    constexpr std::optional<TurnaroundPhase> kLoading = TurnaroundPhase::Loading;
}

class TurnaroundKeyJudgementTest final : public QObject
{
    Q_OBJECT

private slots:
    static void identicalKeysAreTheSame();
    static void onlyTheCouatlDifferingAsksThePilot();
    static void anotherAircraftIdIsDifferent();
    static void anotherAircraftTitleIsDifferent();
    static void anotherAircraftWinsOverTheCouatl();
    static void anotherParkingOnTheGroundIsDifferent();
    static void anotherAirportOnTheGroundIsDifferent();
    static void aLiveAircraftThatIsNotResolvedCannotBeJudged();
    static void aLiveTitleThatIsNotReadCannotBeJudged();
    static void anotherTitleIsDifferentWhileTheLiveAircraftIsNotResolved();
    static void aLiveCouatlThatIsNotKnownCannotBeJudged();
    static void anotherAircraftIsJudgedBeforeTheCouatlArrives();
    static void aSavedParkingWithTheLiveOneEmptyOnTheGroundWaits();
    static void aSavedParkingWithTheLiveOneEmptyWaitsInEveryGroundPhase();
    static void aSavedParkingWithTheLiveOneEmptyDoesNotWaitWithoutAPhase();
    static void aSavedParkingWithTheLiveOneEmptyDoesNotWaitInFlight();
    static void anEmptySavedParkingNeitherFailsNorWaits();
    static void anEmptyLiveAirportDoesNotFailTheKey();
    static void anEmptySavedAirportDoesNotFailTheKey();
    static void theStandIsNotComparedBetweenDepartureAndEngineShutdown_data();
    static void theStandIsNotComparedBetweenDepartureAndEngineShutdown();
    static void theStandIsComparedJustOutsideTheFlightPhases_data();
    static void theStandIsComparedJustOutsideTheFlightPhases();
    static void aFlownFileStillAsksWhenTheCouatlDiffers();
    static void aFlownFileStillFailsOnAnotherAircraft();
    static void aKeyOnlyFileFailsOnAnotherStandWhenBothAreKnown();
    static void aKeyOnlyFileNeverWaitsForTheStand();
};

void TurnaroundKeyJudgementTest::identicalKeysAreTheSame()
{
    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), LiveKey(), kLoading), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::onlyTheCouatlDifferingAsksThePilot()
{
    TurnaroundKey live = LiveKey();
    live.couatlId = kLiveCouatl;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::OnlyCouatlDiffers);
}

void TurnaroundKeyJudgementTest::anotherAircraftIdIsDifferent()
{
    TurnaroundKey live = LiveKey();
    live.aircraftId = kOtherAircraftId;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::anotherAircraftTitleIsDifferent()
{
    TurnaroundKey live = LiveKey();
    live.aircraftTitle = kOtherTitle;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::anotherAircraftWinsOverTheCouatl()
{
    TurnaroundKey live = LiveKey();
    live.aircraftId = kOtherAircraftId;
    live.couatlId = kLiveCouatl;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::anotherParkingOnTheGroundIsDifferent()
{
    TurnaroundKey live = LiveKey();
    live.parkingName = kOtherParking;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::anotherAirportOnTheGroundIsDifferent()
{
    TurnaroundKey live = LiveKey();
    live.airportIcao = kOtherAirport;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aLiveAircraftThatIsNotResolvedCannotBeJudged()
{
    TurnaroundKey live = LiveKey();
    live.aircraftId.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::NotYetJudgeable);
}

void TurnaroundKeyJudgementTest::aLiveTitleThatIsNotReadCannotBeJudged()
{
    TurnaroundKey live = LiveKey();
    live.aircraftTitle.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::NotYetJudgeable);
}

void TurnaroundKeyJudgementTest::anotherTitleIsDifferentWhileTheLiveAircraftIsNotResolved()
{
    TurnaroundKey live = LiveKey();
    live.aircraftId.clear();
    live.aircraftTitle = kOtherTitle;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aLiveCouatlThatIsNotKnownCannotBeJudged()
{
    TurnaroundKey live = LiveKey();
    live.couatlId.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::NotYetJudgeable);
}

void TurnaroundKeyJudgementTest::anotherAircraftIsJudgedBeforeTheCouatlArrives()
{
    TurnaroundKey live = LiveKey();
    live.couatlId.clear();
    live.aircraftId = kOtherAircraftId;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aSavedParkingWithTheLiveOneEmptyOnTheGroundWaits()
{
    TurnaroundKey live = LiveKey();
    live.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::NotYetJudgeable);
}

void TurnaroundKeyJudgementTest::aSavedParkingWithTheLiveOneEmptyWaitsInEveryGroundPhase()
{
    TurnaroundKey live = LiveKey();
    live.parkingName.clear();

    for (const TurnaroundPhase phase : {TurnaroundPhase::RepositionAircraft, TurnaroundPhase::WaitingForEngines,
                                        TurnaroundPhase::PlaceArrivalGroundEquipment, TurnaroundPhase::CabinServices})
    {
        QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, phase), KeyVerdict::NotYetJudgeable);
    }
}

void TurnaroundKeyJudgementTest::aSavedParkingWithTheLiveOneEmptyDoesNotWaitWithoutAPhase()
{
    TurnaroundKey live = LiveKey();
    live.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, std::nullopt), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::aSavedParkingWithTheLiveOneEmptyDoesNotWaitInFlight()
{
    TurnaroundKey live = LiveKey();
    live.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, TurnaroundPhase::OnFlight), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::anEmptySavedParkingNeitherFailsNorWaits()
{
    TurnaroundKey saved = SavedKey();
    saved.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(saved, LiveKey(), kLoading), KeyVerdict::Same);

    TurnaroundKey live = LiveKey();
    live.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(saved, live, kLoading), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::anEmptyLiveAirportDoesNotFailTheKey()
{
    TurnaroundKey live = LiveKey();
    live.airportIcao.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, kLoading), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::anEmptySavedAirportDoesNotFailTheKey()
{
    TurnaroundKey saved = SavedKey();
    saved.airportIcao.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(saved, LiveKey(), kLoading), KeyVerdict::Same);
}

void TurnaroundKeyJudgementTest::theStandIsNotComparedBetweenDepartureAndEngineShutdown_data()
{
    QTest::addColumn<TurnaroundPhase>("phase");

    QTest::newRow("WaitingDeparture") << TurnaroundPhase::WaitingDeparture;
    QTest::newRow("OnFlight") << TurnaroundPhase::OnFlight;
    QTest::newRow("WaitingEngineShutdown") << TurnaroundPhase::WaitingEngineShutdown;
}

void TurnaroundKeyJudgementTest::theStandIsNotComparedBetweenDepartureAndEngineShutdown()
{
    QFETCH(TurnaroundPhase, phase);

    TurnaroundKey live = LiveKey();
    live.airportIcao = kOtherAirport;
    live.parkingName = kOtherParking;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, phase), KeyVerdict::Same);

    live.couatlId = kLiveCouatl;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, phase), KeyVerdict::OnlyCouatlDiffers);
}

void TurnaroundKeyJudgementTest::theStandIsComparedJustOutsideTheFlightPhases_data()
{
    QTest::addColumn<TurnaroundPhase>("phase");

    QTest::newRow("WaitingForEngines") << TurnaroundPhase::WaitingForEngines;
    QTest::newRow("PlaceArrivalGroundEquipment") << TurnaroundPhase::PlaceArrivalGroundEquipment;
}

void TurnaroundKeyJudgementTest::theStandIsComparedJustOutsideTheFlightPhases()
{
    QFETCH(TurnaroundPhase, phase);

    TurnaroundKey live = LiveKey();
    live.parkingName = kOtherParking;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, phase), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aFlownFileStillAsksWhenTheCouatlDiffers()
{
    TurnaroundKey live = LiveKey();
    live.couatlId = kLiveCouatl;
    live.parkingName.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, TurnaroundPhase::OnFlight),
             KeyVerdict::OnlyCouatlDiffers);
}

void TurnaroundKeyJudgementTest::aFlownFileStillFailsOnAnotherAircraft()
{
    TurnaroundKey live = LiveKey();
    live.aircraftId = kOtherAircraftId;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, TurnaroundPhase::OnFlight), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aKeyOnlyFileFailsOnAnotherStandWhenBothAreKnown()
{
    TurnaroundKey live = LiveKey();
    live.parkingName = kOtherParking;

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, std::nullopt), KeyVerdict::Different);
}

void TurnaroundKeyJudgementTest::aKeyOnlyFileNeverWaitsForTheStand()
{
    TurnaroundKey live = LiveKey();
    live.parkingName.clear();
    live.airportIcao.clear();

    QCOMPARE(TurnaroundKeyJudgement::Judge(SavedKey(), live, std::nullopt), KeyVerdict::Same);
}

QTEST_APPLESS_MAIN(TurnaroundKeyJudgementTest)

#include "tst_turnaround_key_judgement.moc"
