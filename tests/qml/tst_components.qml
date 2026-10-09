import QtQuick
import QtTest
import GsxIntegratorClientTests

TestCase {
    id: testCase
    name: "Components"
    when: windowShown
    width: 400
    height: 300

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

            onActionTriggered: heardActions++
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

    function test_statusChipPaintsWithThemeTokens() {
        const chip = createTemporaryObject(statusChipComponent, testCase, { label: "PHASE", value: "BOARDING" });

        verify(chip);
        compare(chip.valueColor, Theme.text);
        compare(chip.color, Theme.panel);
        compare(chip.border.color, Theme.line);
    }
}
