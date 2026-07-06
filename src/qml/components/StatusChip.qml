import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    property color valueColor: Theme.text

    implicitHeight: column.implicitHeight + 14
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: 1

    Column {
        id: column
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 2

        Text {
            text: root.label
            color: Theme.muted
            font.pixelSize: 9
            font.letterSpacing: 1.1
            font.capitalization: Font.AllUppercase
        }

        Text {
            width: parent.width
            text: root.value
            color: root.valueColor
            font.pixelSize: 11
            font.bold: true
            font.capitalization: Font.AllUppercase
            elide: Text.ElideRight
        }
    }
}
