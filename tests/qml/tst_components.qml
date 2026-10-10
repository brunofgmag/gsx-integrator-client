import QtQuick
import QtTest
import GsxIntegratorClientTests

TestCase {
    id: testCase
    name: "Components"
    when: windowShown
    visible: true
    width: 400
    height: 700

    Component {
        id: keyValueRowComponent
        KeyValueRow {
            width: 200
        }
    }

    Component {
        id: switchRowComponent
        SwitchRow {
            width: 200

            property int heardCount: 0
            property bool heardValue: false

            onToggled: (checked) => { heardCount++; heardValue = checked; }
        }
    }

    Component {
        id: advisoryComponent
        Advisory {
            width: 200

            property int heardActions: 0
            property int heardSecondActions: 0

            onActionTriggered: heardActions++
            onSecondActionTriggered: heardSecondActions++
        }
    }

    Component {
        id: operationsScreenComponent
        OperationsScreen {
            width: 380
        }
    }

    Component {
        id: segmentedControlComponent
        SegmentedControl {
            width: 200

            property int heardIndex: -1

            onActivated: (index) => { heardIndex = index; }
        }
    }

    Component {
        id: statusChipComponent
        StatusChip {
            width: 200
        }
    }

    Component {
        id: profilesPaneComponent
        ProfilesPane {
            width: 400
        }
    }

    Component {
        id: servicesPaneComponent
        ServicesPane {
            width: 400
        }
    }

    Component {
        id: automationPaneComponent
        AutomationPane {
            width: 400
        }
    }

    function paneVm() {
        return {
            profileModel: [],
            profileFuelRateModeIndex: 0,
            profileFuelRateText: "",
            profileRecommendedFuelRateText: "",
            profileFuelBadge: "",
            fuelRateUnitText: "",
            selectedProfileIndex: 0,
            detectedProfileIndex: 0,
            profileUseGlobal: false,
            profileFuelEditable: false,
            profileFuelRateEditable: false,
            weightIsLb: false,
            profileSkipReposition: false,
            profileSkipRepositionOnNewTurnaround: true,
            profileSkipRepositionOnNewTurnaroundLocked: false,
            profileCallGpu: false,
            profilePlaceChocks: false,
            profileCallGpuOnArrival: false,
            profilePlaceChocksOnArrival: false,
            profileCallCatering: false,
            profileCallBoardingEarly: false,
            profileCallLavatory: false,
            profileCallWater: false,
            profileCallCleaning: false,
            callGpu: false,
            placeChocks: false,
            callGpuOnArrival: false,
            placeChocksOnArrival: false,
            callCatering: false,
            callBoardingEarly: false,
            callLavatory: false,
            callWater: false,
            callCleaning: false,
            useAircraftStairs: false,
            autoDeice: false,
            crewBoarding: 0,
            crewDeboarding: 0,
            selectDetectedProfile: () => {}
        };
    }

    function automationVm(stored, locked) {
        return {
            fuelRateModeIndex: 0,
            fuelRateEditable: false,
            fuelRateText: "",
            fuelRateUnitText: "",
            weightIsLb: false,
            kgToLb: kg => kg,
            autoSelectGsxChoice: false,
            autoStartFlow: false,
            autoStartLoading: false,
            skipReposition: locked,
            skipRepositionOnNewTurnaround: stored,
            skipRepositionOnNewTurnaroundLocked: locked
        };
    }

    function operationsVm(calls, overrides) {
        const flags = [
            "aircraftSupported", "canReloadSimbrief", "cargoAircraft", "cargoDoorStuck", "connected",
            "doorsHoldingPushback", "enabled", "engineConfirmationBlocked", "fuelDidNotStay",
            "fuelPlanOverCapacity", "fuelRequestStalled", "gsxAvailable", "gsxProfileConflict",
            "inDeboardingPhase", "ownStairsWaitingForPressure", "pmdgOptionsConflict", "serviceInterrupted",
            "servicesStalled", "simbriefError", "simbriefReady"
        ];
        const numbers = ["boardingProgress", "deboardingProgress", "fuelProgress"];
        const texts = [
            "aircraftLabel", "aircraftNameText", "cargoDoorAdvisoryText", "commandError", "commandErrorLabel",
            "dismissAdvisoryLabel", "engineConfirmationAdvisoryText", "fuelCardLabel", "fuelPlanAdvisoryText",
            "fuelProgressText", "fuelRateLabel", "fuelRateText", "fuelRequestAdvisoryText",
            "fuelStayAdvisoryText", "gsxLabel", "gsxProfileActionLabel", "gsxProfileAdvisoryText",
            "gsxStatusText", "holdCountdownText", "loadedFuelLabel", "loadedFuelText", "loadingModeLabel",
            "loadingModeText", "nextPhaseText", "openDoorAdvisoryText", "ownStairsPressureAdvisoryText",
            "paxCardLabel", "paxCountText", "paxLabel", "paxProgressText", "phaseCounterText", "phaseTip",
            "plannedFuelLabel", "plannedFuelText", "plannedPaxLabel", "plannedPaxText", "plannedZfwLabel",
            "plannedZfwText", "pmdgOptionsActionLabel", "pmdgOptionsAdvisoryText", "reloadSimbriefLabel",
            "restartFlowLabel", "resumeDecisionAdvisoryText", "resumeTurnaroundLabel",
            "serviceInterruptedAdvisoryText", "servicesAdvisoryText", "simLabel", "simStatusText",
            "simbriefCardLabel", "simbriefFailureText", "simbriefStatusText", "stateText", "targetFuelLabel",
            "targetFuelText", "targetZfwLabel", "targetZfwText", "turnaroundModeLabel", "turnaroundModeText",
            "turnaroundStateLabel"
        ];
        const vm = {
            dismissFuelStayAdvisory: () => {},
            fixGsxProfile: () => {},
            fixPmdgOptions: () => {},
            reloadSimbrief: () => {},
            restartFlow: () => { calls.restart++; },
            resumeSavedTurnaround: () => { calls.resume++; }
        };

        flags.forEach((name) => { vm[name] = false; });
        numbers.forEach((name) => { vm[name] = 0; });
        texts.forEach((name) => { vm[name] = ""; });
        vm.connected = true;

        return Object.assign(vm, overrides);
    }

    function findButtonByText(item, text) {
        if (item.text === text && typeof item.clicked === "function")
            return item;

        const kids = item.children;

        for (let i = 0; i < kids.length; i++) {
            const hit = findButtonByText(kids[i], text);

            if (hit)
                return hit;
        }

        return null;
    }

    function visibleButtons(item) {
        const found = [];

        if (item.visible && typeof item.clicked === "function")
            found.push(item);

        const kids = item.children;

        for (let i = 0; i < kids.length; i++)
            visibleButtons(kids[i]).forEach((button) => { found.push(button); });

        return found;
    }

    function findRowByTitle(item, title) {
        if (item.title === title && item.helpText !== undefined)
            return item;

        const kids = item.children;

        for (let i = 0; i < kids.length; i++) {
            const hit = findRowByTitle(kids[i], title);

            if (hit)
                return hit;
        }

        return null;
    }

    function test_profilesPaneChocksRowsExplainThemLikeTheServicesPane() {
        const vm = paneVm();
        const profiles = createTemporaryObject(profilesPaneComponent, testCase, { settingsVm: vm });
        const services = createTemporaryObject(servicesPaneComponent, testCase, { settingsVm: vm });

        verify(profiles);
        verify(services);

        const titles = ["Place chocks", "Place chocks on arrival"];

        for (let i = 0; i < titles.length; i++) {
            const profileRow = findRowByTitle(profiles, titles[i]);
            const serviceRow = findRowByTitle(services, titles[i]);

            verify(profileRow, titles[i]);
            verify(serviceRow, titles[i]);
            verify(serviceRow.helpText.length > 0, titles[i]);
            verify(profileRow.helpText.length > 0, titles[i]);
            compare(profileRow.helpText, serviceRow.helpText, titles[i]);
        }
    }

    readonly property string newTurnaroundTitle: "Skip repositioning on a new turnaround"

    function test_automationPaneLocksTheNewTurnaroundOptionOnWhileRepositioningIsSkipped() {
        const pane = createTemporaryObject(automationPaneComponent, testCase,
                                           { settingsVm: automationVm(false, true) });

        verify(pane);

        const row = findRowByTitle(pane, newTurnaroundTitle);

        verify(row);
        compare(row.checked, true);
        compare(row.enabled, false);
    }

    function test_automationPaneShowsTheStoredNewTurnaroundValueOnceUnlocked() {
        const off = createTemporaryObject(automationPaneComponent, testCase,
                                          { settingsVm: automationVm(false, false) });
        const on = createTemporaryObject(automationPaneComponent, testCase,
                                         { settingsVm: automationVm(true, false) });

        verify(off);
        verify(on);

        const offRow = findRowByTitle(off, newTurnaroundTitle);
        const onRow = findRowByTitle(on, newTurnaroundTitle);

        verify(offRow);
        verify(onRow);
        compare(offRow.checked, false);
        compare(offRow.enabled, true);
        compare(onRow.checked, true);
        compare(onRow.enabled, true);
    }

    function test_automationPaneNewTurnaroundToggleReachesTheViewModel() {
        const pane = createTemporaryObject(automationPaneComponent, testCase,
                                           { settingsVm: automationVm(false, false) });

        verify(pane);

        findRowByTitle(pane, newTurnaroundTitle).toggled(true);

        compare(pane.settingsVm.skipRepositionOnNewTurnaround, true);
    }

    function test_profilesPaneLocksTheNewTurnaroundOptionOnWhileTheProfileSkipsRepositioning() {
        const vm = paneVm();
        vm.profileUseGlobal = false;
        vm.profileSkipRepositionOnNewTurnaround = false;
        vm.profileSkipRepositionOnNewTurnaroundLocked = true;
        const locked = createTemporaryObject(profilesPaneComponent, testCase, { settingsVm: vm });

        verify(locked);

        const lockedRow = findRowByTitle(locked, newTurnaroundTitle);

        verify(lockedRow);
        compare(lockedRow.checked, true);
        compare(lockedRow.enabled, false);

        vm.profileSkipRepositionOnNewTurnaroundLocked = false;
        const unlocked = createTemporaryObject(profilesPaneComponent, testCase, { settingsVm: vm });
        const unlockedRow = findRowByTitle(unlocked, newTurnaroundTitle);

        verify(unlockedRow);
        compare(unlockedRow.checked, false);
        compare(unlockedRow.enabled, true);
    }

    function test_profilesPaneNewTurnaroundToggleReachesTheViewModel() {
        const vm = paneVm();
        vm.profileUseGlobal = false;
        const pane = createTemporaryObject(profilesPaneComponent, testCase, { settingsVm: vm });

        verify(pane);

        findRowByTitle(pane, newTurnaroundTitle).toggled(false);

        compare(pane.settingsVm.profileSkipRepositionOnNewTurnaround, false);
    }

    function test_keyValueRowShowsWhatItIsGiven() {
        const row = createTemporaryObject(keyValueRowComponent, testCase, { label: "Fuel", value: "12 000 KG" });

        verify(row);
        compare(row.label, "Fuel");
        compare(row.value, "12 000 KG");
        compare(row.valueColor, Theme.text);
    }

    function test_switchRowSignalReachesItsHandler() {
        const row = createTemporaryObject(switchRowComponent, testCase, { checked: false });

        verify(row);

        row.toggled(true);

        compare(row.heardCount, 1);
        compare(row.heardValue, true);
    }

    function test_segmentedControlSignalReachesItsHandler() {
        const control = createTemporaryObject(segmentedControlComponent, testCase,
                                              { model: ["KG", "LB"], currentIndex: 0 });

        verify(control);
        compare(control.currentIndex, 0);

        control.activated(1);

        compare(control.heardIndex, 1);
    }

    function test_advisoryActionReachesItsHandler() {
        const advisory = createTemporaryObject(advisoryComponent, testCase,
                                               { text: "Fix the GSX profile", actionText: "FIX" });

        verify(advisory);

        advisory.actionTriggered();

        compare(advisory.heardActions, 1);
    }

    function test_advisoryWithTwoActionsRoutesEachClickToItsOwnHandler() {
        const advisory = createTemporaryObject(advisoryComponent, testCase, {
            width: 380,
            text: "GSX restarted since this turnaround was saved.",
            actionText: "Resume",
            secondActionText: "Restart"
        });

        verify(advisory);

        const first = findButtonByText(advisory, "Resume");
        const second = findButtonByText(advisory, "Restart");

        verify(first);
        verify(second);
        verify(first.visible);
        verify(second.visible);
        verify(!first.secondary);
        verify(second.secondary);

        mouseClick(first);

        compare(advisory.heardActions, 1);
        compare(advisory.heardSecondActions, 0);

        mouseClick(second);

        compare(advisory.heardActions, 1);
        compare(advisory.heardSecondActions, 1);
    }

    function test_advisoryWithOneActionShowsNoSecondButton() {
        const advisory = createTemporaryObject(advisoryComponent, testCase, {
            width: 380,
            text: "Fix the GSX profile",
            actionText: "FIX"
        });

        verify(advisory);

        const only = findButtonByText(advisory, "FIX");

        verify(only);
        verify(only.visible);
        compare(visibleButtons(advisory).length, 1);

        mouseClick(only);

        compare(advisory.heardActions, 1);
        compare(advisory.heardSecondActions, 0);
    }

    function test_operationsScreenAsksTheResumeDecisionWithTwoButtons() {
        const calls = { resume: 0, restart: 0 };
        const vm = operationsVm(calls, {
            resumeDecisionAdvisoryText: "GSX restarted since this turnaround was saved.",
            resumeTurnaroundLabel: "Resume turnaround",
            restartFlowLabel: "Restart Flow"
        });
        const screen = createTemporaryObject(operationsScreenComponent, testCase,
                                             { integratorVm: vm, settingsVm: { autoStartLoading: false } });

        verify(screen);

        const resume = findButtonByText(screen, "Resume turnaround");
        const restart = findButtonByText(screen, "Restart Flow");

        verify(resume);
        verify(restart);
        verify(resume.visible);
        verify(restart.visible);

        mouseClick(resume);

        compare(calls.resume, 1);
        compare(calls.restart, 0);

        mouseClick(restart);

        compare(calls.resume, 1);
        compare(calls.restart, 1);
    }

    function test_operationsScreenHidesBothResumeButtonsOutsideTheDecision() {
        const calls = { resume: 0, restart: 0 };
        const vm = operationsVm(calls, {
            restartFlowLabel: "Restart Flow",
            resumeTurnaroundLabel: "Resume turnaround"
        });
        const screen = createTemporaryObject(operationsScreenComponent, testCase,
                                             { integratorVm: vm, settingsVm: { autoStartLoading: false } });

        verify(screen);

        const resume = findButtonByText(screen, "Resume turnaround");
        const restart = findButtonByText(screen, "Restart Flow");

        verify(resume);
        verify(restart);
        verify(!resume.visible);
        verify(!restart.visible);

        const shown = visibleButtons(screen).map((button) => button.text);

        verify(!shown.includes("Restart Flow"));
        verify(!shown.includes("Resume turnaround"));
    }

    function test_statusChipPaintsWithThemeTokens() {
        const chip = createTemporaryObject(statusChipComponent, testCase, { label: "PHASE", value: "BOARDING" });

        verify(chip);
        compare(chip.valueColor, Theme.text);
        compare(chip.color, Theme.panel);
        compare(chip.border.color, Theme.line);
    }
}
