pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var integratorVm
    required property var settingsVm
    required property bool compact

    readonly property bool deboarding: integratorVm.inDeboardingPhase

    spacing: 10

    property var phaseLabels: root.buildPhaseLabels()

    function buildPhaseLabels() {
        const names = [];
        for (let i = 0; i < root.integratorVm.phaseCount; i++) {
            names.push(root.integratorVm.phaseLabelAt(i));
        }
        return names;
    }

    function formatKg(value) {
        return Number(Math.round(value)).toLocaleString(Qt.locale(), 'f', 0) + " " + qsTr("kg");
    }

    readonly property string nextPhaseLabel: integratorVm.phase + 1 < integratorVm.phaseCount
        ? (phaseLabels[integratorVm.phase + 1] ?? "")
        : qsTr("New session")

    Component.onCompleted: root.integratorVm.SnapshotChanged.connect(() => root.phaseLabels = root.buildPhaseLabels())

    ColumnLayout {
        Layout.fillWidth: true
        visible: root.integratorVm.connected
        spacing: 10

        // SIM / GSX PRO / AIRCRAFT / TURNAROUND / LOADING status strip.
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            columnSpacing: 8
            rowSpacing: 8

            StatusChip {
                Layout.fillWidth: true
                label: qsTr("Sim")
                value: root.integratorVm.connected ? qsTr("Connected") : qsTr("Offline")
                valueColor: root.integratorVm.connected ? Theme.ok : Theme.amber
            }

            StatusChip {
                Layout.fillWidth: true
                label: qsTr("GSX Pro")
                value: root.integratorVm.gsxAvailable ? qsTr("Connected") : qsTr("Offline")
                valueColor: root.integratorVm.gsxAvailable ? Theme.ok : Theme.amber
            }

            StatusChip {
                Layout.fillWidth: true
                label: qsTr("Aircraft")
                value: root.integratorVm.aircraftSupported ? root.integratorVm.aircraftName : qsTr("Standby")
                valueColor: root.integratorVm.aircraftSupported ? Theme.text : Theme.muted
            }

            StatusChip {
                Layout.fillWidth: true
                label: qsTr("Turnaround")
                value: root.settingsVm.autoStartFlow ? qsTr("Auto") : qsTr("Manual")
                valueColor: root.integratorVm.enabled ? Theme.accent : Theme.muted
            }

            StatusChip {
                Layout.fillWidth: true
                label: qsTr("Loading")
                value: root.settingsVm.autoStartLoading ? qsTr("Auto") : qsTr("Manual")
                valueColor: root.settingsVm.autoStartLoading ? Theme.accent : Theme.muted
            }
        }

        // Big turnaround state readout.
        DataCard {
            Layout.fillWidth: true
            title: qsTr("Turnaround state")
            helpText: ""
            metric: (root.integratorVm.phase + 1) + "/" + root.integratorVm.phaseCount
            metricColor: Theme.muted

            Text {
                width: parent.width
                text: root.integratorVm.stateText
                color: Theme.text
                font.pixelSize: root.compact ? 19 : 24
                font.bold: true
                font.letterSpacing: 1.5
                font.capitalization: Font.AllUppercase
                wrapMode: Text.WordWrap
            }

            Item { width: 1; height: 4 }

            Item {
                width: parent.width
                height: nextLabel.implicitHeight

                Text {
                    id: nextLabel
                    anchors.left: parent.left
                    anchors.right: holdCountdown.visible ? holdCountdown.left : parent.right
                    anchors.rightMargin: holdCountdown.visible ? 10 : 0
                    text: qsTr("Next") + " ▸ " + root.nextPhaseLabel
                    color: Theme.muted
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    font.capitalization: Font.AllUppercase
                    elide: Text.ElideRight
                }

                Text {
                    id: holdCountdown
                    anchors.right: parent.right
                    visible: root.integratorVm.delayTicksRemaining > 0
                    text: qsTr("Next state in %1s").arg(root.integratorVm.delayTicksRemaining)
                    color: Theme.accent
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    font.capitalization: Font.AllUppercase
                }
            }
        }

        Advisory {
            Layout.fillWidth: true
            visible: root.integratorVm.gsxProfileConflict
            text: root.integratorVm.gsxProfileFixable
                  ? qsTr("The GSX profile for this aircraft does not set 'refueling = 0', so the fuel truck never connects the hose. Apply the fix, then restart GSX or reload the flight.")
                  : qsTr("No GSX profile with 'refueling = 0' was found for this aircraft. Install an aircraft profile and set 'refueling = 0' in its gsx.cfg.")
            actionText: root.integratorVm.gsxProfileFixable ? qsTr("Fix profile") : ""
            onActionTriggered: root.integratorVm.fixGsxProfile()
        }

        Advisory {
            Layout.fillWidth: true
            text: root.integratorVm.phaseTip
        }

        Rectangle {
            Layout.fillWidth: true
            visible: root.integratorVm.commandError.length > 0
            implicitHeight: Math.max(errorRow.implicitHeight + 18, 38)
            radius: Theme.radius
            color: "transparent"
            border.color: Theme.red
            border.width: 1

            Row {
                id: errorRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: errorBadge.implicitWidth + 14
                    height: errorBadge.implicitHeight + 6
                    radius: Theme.radiusSmall
                    color: Theme.red

                    Text {
                        id: errorBadge
                        anchors.centerIn: parent
                        text: qsTr("Error")
                        color: Theme.bg
                        font.pixelSize: 9
                        font.bold: true
                        font.letterSpacing: 1.2
                        font.capitalization: Font.AllUppercase
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - errorBadge.implicitWidth - 24
                    text: root.integratorVm.commandError
                    color: Theme.red
                    font.pixelSize: 11
                    lineHeight: 1.3
                    wrapMode: Text.WordWrap
                }
            }
        }

        // FUEL / BOARDING / SIMBRIEF OFP data grid.
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            columns: root.compact ? 1 : 3
            columnSpacing: 10
            rowSpacing: 10

            DataCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Fuel")
                metric: Math.round(root.integratorVm.fuelProgress) + "%"
                progress: root.integratorVm.fuelProgress

                KeyValueRow {
                    label: qsTr("Loaded")
                    value: root.formatKg(root.integratorVm.loadedFuelKg)
                }
                KeyValueRow {
                    label: qsTr("Planned")
                    value: root.formatKg(root.integratorVm.targetFuelKg)
                }
                KeyValueRow {
                    label: qsTr("Rate")
                    value: root.integratorVm.refuelByGsx
                        ? qsTr("Auto")
                        : root.integratorVm.refuelBySelf
                            ? "GSX"
                            : root.settingsVm.fuelRateText + " " + qsTr("kg/s")
                }
            }

            DataCard {
                id: paxCard
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property double paxProgress: root.deboarding
                    ? root.integratorVm.deboardingProgress
                    : root.integratorVm.boardingProgress
                title: root.deboarding ? qsTr("Deboarding") : qsTr("Boarding")
                metric: Math.round(paxCard.paxProgress) + "%"
                progress: paxCard.paxProgress

                KeyValueRow {
                    visible: !root.integratorVm.cargoAircraft
                    label: qsTr("Pax")
                    value: (root.deboarding
                            ? Math.round(paxCard.paxProgress / 100 * root.integratorVm.targetPax)
                            : root.integratorVm.boardedPax)
                           + " / " + root.integratorVm.targetPax
                }
                KeyValueRow {
                    label: qsTr("Planned ZFW")
                    value: root.formatKg(root.integratorVm.targetZfwKg)
                }
            }

            DataCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("SimBrief OFP")
                helpText: ""
                metric: root.integratorVm.simbriefStatusText
                metricColor: root.integratorVm.simbriefReady
                    ? Theme.ok
                    : (root.integratorVm.simbriefError ? Theme.red : Theme.muted)

                KeyValueRow {
                    label: qsTr("Fuel")
                    value: root.formatKg(root.integratorVm.plannedFuelKg)
                }
                KeyValueRow {
                    label: qsTr("ZFW")
                    value: root.formatKg(root.integratorVm.plannedZfwKg)
                }
                KeyValueRow {
                    label: qsTr("Pax")
                    value: String(root.integratorVm.plannedPax)
                }

                Item { width: 1; height: 6 }

                ActionButton {
                    width: parent.width
                    small: true
                    text: qsTr("Reload SimBrief")
                    enabled: root.integratorVm.canReloadSimbrief
                    onClicked: root.integratorVm.reloadSimbrief()
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignLeft
            spacing: 8

            ActionButton {
                small: true
                text: qsTr("Start Flow")
                enabled: root.integratorVm.canToggleAutomation && !root.integratorVm.enabled && !root.settingsVm.autoStartFlow
                onClicked: root.integratorVm.startFlow()
            }

            ActionButton {
                small: true
                text: qsTr("Start Loading")
                enabled: root.integratorVm.canStartLoading
                onClicked: root.integratorVm.startLoading()
            }

            ActionButton {
                id: restartButton

                property bool armed: false

                small: true
                secondary: !restartButton.armed
                tint: Theme.red
                text: restartButton.armed ? qsTr("Confirm restart") : qsTr("Restart Flow")
                enabled: root.integratorVm.connected && root.integratorVm.enabled
                onEnabledChanged: restartButton.armed = false
                onClicked: {
                    if (restartButton.armed) {
                        restartButton.armed = false;
                        root.integratorVm.restartFlow();
                    } else {
                        restartButton.armed = true;
                        disarmTimer.restart();
                    }
                }

                Timer {
                    id: disarmTimer
                    interval: 3000
                    onTriggered: restartButton.armed = false
                }
            }

            ActionButton {
                small: true
                secondary: true
                visible: root.integratorVm.debugToolsAvailable
                text: "◂ Phase"
                onClicked: root.integratorVm.debugSkipPhase(-1)
            }

            ActionButton {
                small: true
                secondary: true
                visible: root.integratorVm.debugToolsAvailable
                text: "Phase ▸"
                onClicked: root.integratorVm.debugSkipPhase(1)
            }
        }
    }
}
