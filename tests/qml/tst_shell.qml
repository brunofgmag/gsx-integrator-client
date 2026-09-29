import QtQuick
import QtQuick.Controls.Basic
import QtTest
import GsxIntegratorClientTests

TestCase {
    id: testCase
    name: "Shell"
    when: windowShown
    width: 400
    height: 300

    Component {
        id: fakeIntegratorComponent
        FakeIntegratorVm {}
    }

    Component {
        id: shellComponent
        Main {
            settingsVm: testCase.lenientVm({ activeRenderer: "software", effectiveDark: true, closeToTray: false,
                                             minimizeToTray: false, trayTipShown: true, canSave: false,
                                             validationMessage: "", saveMessage: "", saveError: false })
            updateVm: testCase.lenientVm({ updateAvailable: false, commbusUpdateAvailable: false,
                                           readyToRestart: false, downloadsAllowed: false, addonsAvailable: false })
            startHidden: false
            startMinimized: false
            trayIconSource: ""
        }
    }

    function lenientVm(overrides) {
        return new Proxy(overrides, {
            get(target, key) {
                if (typeof key === "symbol")
                    return undefined
                return key in target ? target[key] : ""
            }
        })
    }

    function openShell() {
        const fake = createTemporaryObject(fakeIntegratorComponent, testCase)
        verify(fake)
        const shell = createTemporaryObject(shellComponent, null, { integratorVm: fake })
        verify(shell)
        shell.show()
        tryVerify(() => shell.visible)
        wait(300)
        return shell
    }

    function findByText(item, text) {
        if (item.text === text && item.clicked !== undefined)
            return item
        const kids = item.children
        for (let i = 0; i < kids.length; i++) {
            const hit = findByText(kids[i], text)
            if (hit)
                return hit
        }
        return null
    }

    function findFlickable(item) {
        if (item.contentY !== undefined && item.flickableDirection !== undefined && item.visible)
            return item
        const kids = item.children
        for (let i = 0; i < kids.length; i++) {
            const hit = findFlickable(kids[i])
            if (hit)
                return hit
        }
        return null
    }

    function findScrollBar(flick) {
        const kids = flick.children
        for (let i = 0; i < kids.length; i++) {
            if (kids[i].policy !== undefined && kids[i].orientation !== undefined)
                return kids[i]
        }
        return null
    }

    function openScrollingShell(phase, height) {
        const shell = openShell()
        shell.integratorVm.phase = phase
        shell.height = height
        wait(400)
        const flick = findFlickable(shell.contentItem)
        verify(flick)
        const bar = findScrollBar(flick)
        verify(bar)
        return { shell: shell, flick: flick, bar: bar }
    }

    function findFlickables(item, found) {
        if (item.contentY !== undefined && item.flickableDirection !== undefined && item.visible)
            found.push(item)
        const kids = item.children
        for (let i = 0; i < kids.length; i++)
            findFlickables(kids[i], found)
        return found
    }

    function findRailItem(item, text) {
        if (item.text === text && item.clicked === undefined && item.parent && item.parent.radius !== undefined)
            return item.parent
        const kids = item.children
        for (let i = 0; i < kids.length; i++) {
            const hit = findRailItem(kids[i], text)
            if (hit)
                return hit
        }
        return null
    }

    function openSettings(height) {
        const shell = openShell()
        shell.integratorVm.phase = 1
        shell.height = height
        shell.screen = 1
        wait(400)
        const flicks = findFlickables(shell.contentItem, [])
        compare(flicks.length, 2)
        return { shell: shell, outer: flicks[0], inner: flicks[1], settings: flicks[1].parent.parent }
    }

    function tallestSection(parts) {
        let best = 0
        let bestHeight = 0
        for (let i = 0; i < 6; i++) {
            parts.settings.sectionIndex = i
            wait(100)
            if (parts.inner.contentHeight > bestHeight) {
                best = i
                bestHeight = parts.inner.contentHeight
            }
        }
        parts.settings.sectionIndex = best
        wait(200)
        return best
    }

    function test_scrollBarIsAlwaysOnWhenTheContentIsTallerThanTheViewport() {
        const parts = openScrollingShell(1, 500)

        verify(parts.flick.contentHeight > parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOn)
        verify(parts.bar.visible)
        compare(parts.bar.width, 10)
        const stack = parts.flick.contentItem.children[0]
        verify(stack.x + stack.width + 4 <= parts.bar.x)
    }

    function test_scrollBarIsHiddenWhenTheContentFits() {
        const parts = openScrollingShell(0, 1000)

        verify(parts.flick.contentHeight <= parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOff)
        verify(!parts.bar.visible)
    }

    function test_scrollBarIsHiddenWhenOnlyTheBottomPaddingOverflows() {
        const parts = openScrollingShell(1, 1000)
        const chrome = parts.shell.height - parts.flick.height
        parts.shell.height = parts.flick.contentHeight - 8 + chrome
        wait(400)

        const stack = parts.flick.contentItem.children[0]
        verify(parts.flick.contentHeight > parts.flick.height)
        verify(stack.y + stack.implicitHeight <= parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOff)
        verify(!parts.bar.visible)
        verify(!parts.flick.interactive)
        mouseWheel(parts.flick, 100, 100, 0, -120)
        wait(200)
        compare(parts.flick.contentY, 0)
    }

    function test_scrollBarIsHiddenAtTheDefaultHeightWithTheShortPhase() {
        const parts = openScrollingShell(0, 640)

        compare(parts.shell.height, 640)
        verify(parts.flick.contentHeight < parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOff)
        verify(!parts.bar.visible)
    }

    function test_scrollBarIsHiddenAtTheDefaultHeightOnTheSettingsScreen() {
        const parts = openScrollingShell(1, 640)

        parts.shell.screen = 1
        wait(400)

        compare(parts.shell.height, 640)
        verify(parts.flick.contentHeight <= parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOff)
        verify(!parts.bar.visible)
    }

    function test_scrollBarIsAlwaysOnAtTheDefaultHeightWithTheLongPhase() {
        const parts = openScrollingShell(1, 640)

        compare(parts.shell.height, 640)
        verify(parts.flick.contentHeight > parts.flick.height)
        compare(parts.bar.policy, ScrollBar.AlwaysOn)
        verify(parts.bar.visible)
    }

    function test_settingsPaneScrollsWhileTheRailAndTheOuterFlickStayStill() {
        const parts = openSettings(500)
        tallestSection(parts)

        const outerBar = findScrollBar(parts.outer)
        const innerBar = findScrollBar(parts.inner)
        verify(innerBar)
        verify(parts.inner.contentHeight > parts.inner.height)
        compare(innerBar.policy, ScrollBar.AlwaysOn)
        verify(innerBar.visible)
        compare(innerBar.width, 10)
        const paneStack = parts.inner.contentItem.children[0]
        verify(paneStack.x + paneStack.width + 4 <= innerBar.x)
        compare(outerBar.policy, ScrollBar.AlwaysOff)
        verify(!outerBar.visible)
        verify(!parts.outer.interactive)
        compare(parts.outer.contentHeight, parts.outer.height)

        const rail = findRailItem(parts.shell.contentItem, "Services")
        verify(rail)
        const before = rail.mapToItem(parts.shell.contentItem, 0, 0)

        compare(parts.inner.contentY, 0)
        mouseWheel(parts.inner, 2, 2, 0, -120)
        tryVerify(() => parts.inner.contentY > 0)

        const after = rail.mapToItem(parts.shell.contentItem, 0, 0)
        compare(after.y, before.y)
        compare(after.x, before.x)
        compare(parts.outer.contentY, 0)

        parts.settings.sectionIndex = 0
        wait(200)
        compare(parts.inner.contentY, 0)
    }

    function test_settingsRailKeepsItsWidthAndThePaneTakesTheRest() {
        const parts = openSettings(640)
        const rail = findRailItem(parts.shell.contentItem, "General")
        verify(rail)

        compare(rail.width, 116)
        verify(parts.inner.width > parts.settings.width - 116 - 24)
        const paneStack = parts.inner.contentItem.children[0]
        verify(paneStack.width > parts.settings.width / 2)
    }

    function test_settingsShortPaneHasNoInnerScrollBarAtTheDefaultHeight() {
        const parts = openSettings(640)
        parts.settings.sectionIndex = 4
        wait(200)

        const innerBar = findScrollBar(parts.inner)
        verify(innerBar)
        verify(parts.inner.contentHeight <= parts.inner.height)
        compare(innerBar.policy, ScrollBar.AlwaysOff)
        verify(!innerBar.visible)
        verify(!parts.inner.interactive)
        const outerBar = findScrollBar(parts.outer)
        verify(!outerBar.visible)
        verify(!parts.outer.interactive)
    }

    function test_scrollBarTrackClickAndMouseWheelScroll() {
        const parts = openScrollingShell(1, 500)

        compare(parts.flick.contentY, 0)
        mouseWheel(parts.flick, 100, 100, 0, -120)
        tryVerify(() => parts.flick.contentY > 0)

        parts.flick.contentY = 0
        mouseClick(parts.bar, parts.bar.width / 2, parts.bar.height - 6)
        tryVerify(() => parts.flick.contentY > 0)
    }

    function test_windowSizeDoesNotFollowThePhase() {
        const shell = openShell()
        const fake = shell.integratorVm
        const before = { width: shell.width, height: shell.height }

        fake.phase = 1
        wait(400)
        compare(shell.width, before.width)
        compare(shell.height, before.height)

        fake.phase = 2
        wait(400)
        compare(shell.width, before.width)
        compare(shell.height, before.height)
    }

    function test_twoTapsInARowOnPreviousPhaseIssueTwoRequests() {
        const shell = openShell()
        const fake = shell.integratorVm
        const button = findByText(shell.contentItem, "◂ Phase")
        verify(button)
        const spot = button.mapToItem(shell.contentItem, button.width / 2, button.height / 2)

        mouseClick(shell.contentItem, spot.x, spot.y)
        wait(150)
        mouseClick(shell.contentItem, spot.x, spot.y)
        wait(150)

        compare(fake.skipCalls, 2)
        compare(fake.lastDelta, -1)
        compare(fake.phase, -2)
    }

    function test_aDoubleClickOnPreviousPhaseIssuesTwoRequests() {
        const shell = openShell()
        const fake = shell.integratorVm
        const button = findByText(shell.contentItem, "◂ Phase")
        verify(button)
        const spot = button.mapToItem(shell.contentItem, button.width / 2, button.height / 2)

        mouseDoubleClickSequence(shell.contentItem, spot.x, spot.y)
        wait(150)

        compare(fake.skipCalls, 2)
    }
}
