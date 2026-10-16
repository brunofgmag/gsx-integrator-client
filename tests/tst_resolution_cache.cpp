#include <QtTest/QTest>

#include "../src/infrastructure/simulator/ResolutionCache.h"

namespace
{
    using Clock = ResolutionCache::Clock;

    const Clock::time_point kStart = Clock::time_point{} + std::chrono::hours(1);
}

class ResolutionCacheTest final : public QObject
{
    Q_OBJECT

private slots:
    static void aFoundValueIsResolvedOnceUntilReset();
    static void anEmptyResultIsResolvedAgainAfterItsShortLifetime();
    static void aDifferentKeyResolvesAgain();
    static void resetForcesANewResolution();
};

void ResolutionCacheTest::aFoundValueIsResolvedOnceUntilReset()
{
    ResolutionCache cache;
    int calls = 0;
    const auto resolve = [&calls] { ++calls; return QStringLiteral("roaming"); };

    QCOMPARE(cache.Resolve(QStringLiteral("home"), resolve, kStart), QStringLiteral("roaming"));
    QCOMPARE(cache.Resolve(QStringLiteral("home"), resolve, kStart + std::chrono::hours(5)), QStringLiteral("roaming"));
    QCOMPARE(calls, 1);
}

void ResolutionCacheTest::anEmptyResultIsResolvedAgainAfterItsShortLifetime()
{
    ResolutionCache cache;
    int calls = 0;
    const auto resolve = [&calls] { ++calls; return QString(); };

    QVERIFY(cache.Resolve(QStringLiteral("home"), resolve, kStart).isEmpty());
    QVERIFY(cache.Resolve(QStringLiteral("home"), resolve, kStart + std::chrono::seconds(1)).isEmpty());
    QCOMPARE(calls, 1);

    QVERIFY(cache.Resolve(QStringLiteral("home"), resolve, kStart + ResolutionCache::kEmptyLifetime).isEmpty());
    QCOMPARE(calls, 2);
}

void ResolutionCacheTest::aDifferentKeyResolvesAgain()
{
    ResolutionCache cache;
    int calls = 0;
    const auto resolve = [&calls] { ++calls; return QStringLiteral("value"); };

    QVERIFY(!cache.Resolve(QStringLiteral("a"), resolve, kStart).isEmpty());
    QVERIFY(!cache.Resolve(QStringLiteral("b"), resolve, kStart).isEmpty());
    QCOMPARE(calls, 2);
}

void ResolutionCacheTest::resetForcesANewResolution()
{
    ResolutionCache cache;
    int calls = 0;
    const auto resolve = [&calls] { ++calls; return QStringLiteral("value"); };

    QVERIFY(!cache.Resolve(QStringLiteral("a"), resolve, kStart).isEmpty());
    cache.Reset();
    QVERIFY(!cache.Resolve(QStringLiteral("a"), resolve, kStart).isEmpty());
    QCOMPARE(calls, 2);
}

QTEST_APPLESS_MAIN(ResolutionCacheTest)

#include "tst_resolution_cache.moc"
