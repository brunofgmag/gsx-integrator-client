#include <QtGui/QExposeEvent>
#include <QtGui/QWindow>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../src/infrastructure/platform/MinimizeByExposure.h"

namespace
{
    void SendExpose(QWindow& window)
    {
        QExposeEvent event{QRegion()};
        QCoreApplication::sendEvent(&window, &event);
    }

    void Focus(QWindow& window)
    {
        QWindowSystemInterface::handleFocusWindowChanged<QWindowSystemInterface::SynchronousDelivery>(&window);
    }

    void Unfocus()
    {
        QWindowSystemInterface::handleFocusWindowChanged<QWindowSystemInterface::SynchronousDelivery>(nullptr);
    }
}

class MinimizeByExposureTest final : public QObject
{
    Q_OBJECT

private slots:
    static void aFocusedWindowThatLosesExposureCountsAsMinimized();
    static void aWindowThatNeverHadFocusIsNotMinimized();
    static void aWindowThatLostFocusJustBeforeExposureCountsAsMinimized();
    static void aWindowThatLostFocusLongerAgoThanTheGraceIsNotMinimized();
    static void anExposedWindowIsNotMinimized();
    static void aHiddenWindowIsNotMinimized();
};

void MinimizeByExposureTest::aFocusedWindowThatLosesExposureCountsAsMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    Focus(window);
    QVERIFY(window.isVisible());
    QVERIFY(window.isActive());
    QVERIFY(!window.isExposed());

    SendExpose(window);

    QCOMPARE(minimized.count(), 1);
}

void MinimizeByExposureTest::aWindowThatNeverHadFocusIsNotMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    QVERIFY(window.isVisible());
    QVERIFY(!window.isActive());
    QVERIFY(!window.isExposed());

    SendExpose(window);

    QCOMPARE(minimized.count(), 0);
}

void MinimizeByExposureTest::aWindowThatLostFocusJustBeforeExposureCountsAsMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    Focus(window);
    Unfocus();
    QVERIFY(window.isVisible());
    QVERIFY(!window.isActive());
    QVERIFY(!window.isExposed());

    SendExpose(window);

    QCOMPARE(minimized.count(), 1);
}

void MinimizeByExposureTest::aWindowThatLostFocusLongerAgoThanTheGraceIsNotMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window, std::chrono::milliseconds{20});
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    Focus(window);
    Unfocus();
    QTest::qSleep(80);
    QVERIFY(window.isVisible());
    QVERIFY(!window.isExposed());

    SendExpose(window);

    QCOMPARE(minimized.count(), 0);
}

void MinimizeByExposureTest::anExposedWindowIsNotMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SendExpose(window);

    QCOMPARE(minimized.count(), 0);
}

void MinimizeByExposureTest::aHiddenWindowIsNotMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.hide();
    QTest::qWait(50);

    SendExpose(window);

    QCOMPARE(minimized.count(), 0);
}

QTEST_MAIN(MinimizeByExposureTest)

#include "tst_minimize_by_exposure.moc"
