import QtQuick

Item {
    id: root

    property string label: ""
    property string value: ""
    property color valueColor: Theme.text

    width: parent ? parent.width : implicitWidth
    implicitHeight: 20

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.muted
        font.pixelSize: 10
        font.letterSpacing: 0.6
        font.capitalization: Font.AllUppercase
    }

    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.value
        color: root.valueColor
        font.pixelSize: 10
        font.capitalization: Font.AllUppercase
    }
}
