import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root

    property string text: ""

    spacing: 10

    Text {
        text: root.text
        color: Theme.muted
        font.pixelSize: 9
        font.letterSpacing: 2
        font.capitalization: Font.AllUppercase
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.line
    }
}
