#include <cmath>
#include <limits>

#include <QtTest/QTest>

#include "../src/domain/turnaround/TurnaroundMath.h"

class TurnaroundMathTest final : public QObject
{
    Q_OBJECT

private slots:
    void reportsLinearProgress();
    void clampsBelowZero();
    void clampsAboveHundred();
    void handlesDecreasingTarget();
    void negligibleChangeReturnsHundredWhenAtTarget();
    void negligibleChangeReturnsZeroWhenAwayFromTarget();
    void nonFiniteInputsReturnZero();
    void epsilonBoundaryFiftyKgIsNegligible();
};

void TurnaroundMathTest::reportsLinearProgress()
{
    // initial 1000, target 3000, halfway at 2000 -> 50%.
    QCOMPARE(turnaround::ProgressPercent(1000.0, 2000.0, 3000.0), 50.0);
}

void TurnaroundMathTest::clampsBelowZero()
{
    // current below initial would be negative; clamped to 0.
    QCOMPARE(turnaround::ProgressPercent(1000.0, 500.0, 3000.0), 0.0);
}

void TurnaroundMathTest::clampsAboveHundred()
{
    // current beyond target would exceed 100; clamped to 100.
    QCOMPARE(turnaround::ProgressPercent(1000.0, 4000.0, 3000.0), 100.0);
}

void TurnaroundMathTest::handlesDecreasingTarget()
{
    // Deboarding: weight falls from 3000 toward 1000, halfway at 2000 -> 50%.
    QCOMPARE(turnaround::ProgressPercent(3000.0, 2000.0, 1000.0), 50.0);
}

void TurnaroundMathTest::negligibleChangeReturnsHundredWhenAtTarget()
{
    // |target - initial| <= epsilon (50kg) and current near target -> done.
    QCOMPARE(turnaround::ProgressPercent(1000.0, 1010.0, 1020.0), 100.0);
}

void TurnaroundMathTest::negligibleChangeReturnsZeroWhenAwayFromTarget()
{
    // Negligible planned change but current is far from target -> not started.
    QCOMPARE(turnaround::ProgressPercent(1000.0, 5000.0, 1020.0), 0.0);
}

void TurnaroundMathTest::nonFiniteInputsReturnZero()
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();

    QCOMPARE(turnaround::ProgressPercent(nan, 1000.0, 2000.0), 0.0);
    QCOMPARE(turnaround::ProgressPercent(0.0, nan, 2000.0), 0.0);
    QCOMPARE(turnaround::ProgressPercent(0.0, 1000.0, nan), 0.0);
    QCOMPARE(turnaround::ProgressPercent(inf, 1000.0, 2000.0), 0.0);
    QCOMPARE(turnaround::ProgressPercent(0.0, -inf, 2000.0), 0.0);
}

void TurnaroundMathTest::epsilonBoundaryFiftyKgIsNegligible()
{
    QCOMPARE(turnaround::ProgressPercent(1000.0, 1050.0, 1050.0), 100.0);
    QCOMPARE(turnaround::ProgressPercent(1000.0, 0.0, 1050.0), 0.0);
}

QTEST_APPLESS_MAIN(TurnaroundMathTest)

#include "tst_turnaround_math.moc"
