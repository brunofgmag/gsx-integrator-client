#include <QtGui/QExposeEvent>
#include <QtGui/QWindow>
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
}

class MinimizeByExposureTest final : public QObject
{
    Q_OBJECT

private slots:
    static void aShownWindowThatIsNotExposedCountsAsMinimized();
    static void anExposedWindowIsNotMinimized();
    static void aHiddenWindowIsNotMinimized();
};

void MinimizeByExposureTest::aShownWindowThatIsNotExposedCountsAsMinimized()
{
    QWindow window;
    window.resize(200, 150);
    const MinimizeByExposure watch(&window);
    const QSignalSpy minimized(&watch, &MinimizeByExposure::Minimized);

    window.show();
    QVERIFY(window.isVisible());
    QVERIFY(!window.isExposed());

    SendExpose(window);

    QCOMPARE(minimized.count(), 1);
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
