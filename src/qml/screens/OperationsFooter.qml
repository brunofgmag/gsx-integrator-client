import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property var integratorVm
    required property int shellMargin

    implicitHeight: 60
    color: Theme.panel2

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: root.shellMargin
        anchors.verticalCenter: parent.verticalCenter
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
