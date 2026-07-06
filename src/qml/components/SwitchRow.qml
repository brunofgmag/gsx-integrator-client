import QtQuick

SettingRow {
    id: root

    property bool checked: false
    signal toggled(bool checked)

    SquareSwitch {
        id: toggle
        checked: root.checked
        onClicked: {
            const next = toggle.checked;
            toggle.checked = Qt.binding(() => root.checked);
            root.toggled(next);
        }
    }
}
