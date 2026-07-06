import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string caption: ""
    property string helpText: ""
    default property alias control: controlRow.data

    implicitHeight: Math.max(copyColumn.implicitHeight, controlRow.implicitHeight) + 22
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: 1

    Column {
        id: copyColumn
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.right: controlRow.left
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        Row {
            spacing: 7

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: Theme.text
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase
            }

            HelpHint {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.helpText.length > 0
                text: root.helpText
            }
        }

        Text {
            width: parent.width
            text: root.caption
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 0.4
            font.capitalization: Font.AllUppercase
            wrapMode: Text.WordWrap
            visible: root.caption.length > 0
        }
    }

    Row {
        id: controlRow
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
    }
}
