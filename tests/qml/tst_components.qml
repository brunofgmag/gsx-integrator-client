import QtQuick
import QtTest
import GsxIntegratorClient

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
