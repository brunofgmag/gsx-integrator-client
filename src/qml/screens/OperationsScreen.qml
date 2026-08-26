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
                label: root.integratorVm.simLabel
                value: root.integratorVm.simStatusText
                valueColor: root.integratorVm.connected ? Theme.ok : Theme.amber
            }

            StatusChip {
                Layout.fillWidth: true
                label: root.integratorVm.gsxLabel
                value: root.integratorVm.gsxStatusText
                valueColor: root.integratorVm.gsxAvailable ? Theme.ok : Theme.amber
            }

            StatusChip {
                Layout.fillWidth: true
                label: root.integratorVm.aircraftLabel
                value: root.integratorVm.aircraftNameText
                valueColor: root.integratorVm.aircraftSupported ? Theme.text : Theme.muted
            }

            StatusChip {
                Layout.fillWidth: true
                label: root.integratorVm.turnaroundModeLabel
                value: root.integratorVm.turnaroundModeText
                valueColor: root.integratorVm.enabled ? Theme.accent : Theme.muted
            }

            StatusChip {
                Layout.fillWidth: true
                label: root.integratorVm.loadingModeLabel
                value: root.integratorVm.loadingModeText
                valueColor: root.settingsVm.autoStartLoading ? Theme.accent : Theme.muted
            }
        }

        // Big turnaround state readout.
        DataCard {
            Layout.fillWidth: true
            title: root.integratorVm.turnaroundStateLabel
            helpText: ""
            metric: root.integratorVm.phaseCounterText
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
                    text: root.integratorVm.nextPhaseText
                    color: Theme.muted
                    font.pixelSize: 11
                    font.letterSpacing: 0.8
                    font.capitalization: Font.AllUppercase
                    elide: Text.ElideRight
                }

                Text {
                    id: holdCountdown
                    anchors.right: parent.right
                    visible: root.integratorVm.holdCountdownText !== ""
                    text: root.integratorVm.holdCountdownText
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
            text: root.integratorVm.gsxProfileAdvisoryText
            actionText: root.integratorVm.gsxProfileActionLabel
            onActionTriggered: root.integratorVm.fixGsxProfile()
        }

        Advisory {
            Layout.fillWidth: true
            visible: root.integratorVm.pmdgOptionsConflict
            text: root.integratorVm.pmdgOptionsAdvisoryText
            actionText: root.integratorVm.pmdgOptionsActionLabel
            onActionTriggered: root.integratorVm.fixPmdgOptions()
        }

        Advisory {
            Layout.fillWidth: true
            visible: root.integratorVm.cargoDoorStuck
            text: root.integratorVm.cargoDoorAdvisoryText
        }

        Advisory {
            Layout.fillWidth: true
            visible: root.integratorVm.fuelRequestStalled
            text: root.integratorVm.fuelRequestAdvisoryText
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
                        text: root.integratorVm.commandErrorLabel
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
                title: root.integratorVm.fuelCardLabel
                metric: root.integratorVm.fuelProgressText
                progress: root.integratorVm.fuelProgress

                KeyValueRow {
                    label: root.integratorVm.loadedFuelLabel
                    value: root.integratorVm.loadedFuelText
                }
                KeyValueRow {
                    label: root.integratorVm.targetFuelLabel
                    value: root.integratorVm.targetFuelText
                }
                KeyValueRow {
                    label: root.integratorVm.fuelRateLabel
                    value: root.integratorVm.fuelRateText
                }
            }

            DataCard {
                id: paxCard
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property double paxProgress: root.deboarding
                    ? root.integratorVm.deboardingProgress
                    : root.integratorVm.boardingProgress
                title: root.integratorVm.paxCardLabel
                metric: root.integratorVm.paxProgressText
                progress: paxCard.paxProgress

                KeyValueRow {
                    visible: !root.integratorVm.cargoAircraft
                    label: root.integratorVm.paxLabel
                    value: root.integratorVm.paxCountText
                }
                KeyValueRow {
                    label: root.integratorVm.targetZfwLabel
                    value: root.integratorVm.targetZfwText
                }
            }

            DataCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: root.integratorVm.simbriefCardLabel
                helpText: ""
                metric: root.integratorVm.simbriefStatusText
                metricColor: root.integratorVm.simbriefReady
                    ? Theme.ok
                    : (root.integratorVm.simbriefError ? Theme.red : Theme.muted)

                Item {
                    width: parent ? parent.width : 0
                    implicitHeight: failureText.contentHeight
                    visible: root.integratorVm.simbriefFailureText !== ""

                    Text {
                        id: failureText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        text: root.integratorVm.simbriefFailureText
                        color: Theme.red
                        font.pixelSize: 10
                        font.capitalization: Font.AllUppercase
                        wrapMode: Text.WordWrap
                    }
                }

                KeyValueRow {
                    label: root.integratorVm.plannedFuelLabel
                    value: root.integratorVm.plannedFuelText
                }
                KeyValueRow {
                    label: root.integratorVm.plannedZfwLabel
                    value: root.integratorVm.plannedZfwText
                }
                KeyValueRow {
                    label: root.integratorVm.plannedPaxLabel
                    value: root.integratorVm.plannedPaxText
                }

                Item { width: 1; height: 6 }

                ActionButton {
                    width: parent.width
                    small: true
                    text: root.integratorVm.reloadSimbriefLabel
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
                text: root.integratorVm.startFlowLabel
                enabled: root.integratorVm.canStartFlow
                onClicked: root.integratorVm.startFlow()
            }

            ActionButton {
                small: true
                text: root.integratorVm.startLoadingLabel
                enabled: root.integratorVm.canStartLoading
                onClicked: root.integratorVm.startLoading()
            }

            ActionButton {
                id: restartButton

                property bool armed: false

                small: true
                secondary: !restartButton.armed
                tint: Theme.red
                text: restartButton.armed
                      ? root.integratorVm.confirmRestartLabel
                      : root.integratorVm.restartFlowLabel
                enabled: root.integratorVm.canRestartFlow
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
