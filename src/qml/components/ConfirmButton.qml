import QtQuick

ActionButton {
    id: root

    property string baseText: ""
    property bool armed: false

    signal confirmed()

    text: armed ? qsTr("Confirm?") : baseText
    secondary: !armed
    small: true
    tint: armed ? Theme.amber : Theme.accent

    onClicked: {
        if (root.armed) {
            root.armed = false;
            root.confirmed();
        } else {
            root.armed = true;
        }
    }

    Timer {
        interval: 3000
        running: root.armed
        onTriggered: root.armed = false
    }
}
