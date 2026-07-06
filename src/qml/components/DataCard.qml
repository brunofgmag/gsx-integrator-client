import QtQuick

Rectangle {
    id: root

    property string title: ""
    property string helpText: ""
    property string metric: ""
    property color metricColor: Theme.accent
    property real progress: -1
    default property alias content: contentColumn.data

    implicitHeight: layout.implicitHeight + 26
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: 1

    Column {
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 13
        spacing: 0

        Item {
            width: parent.width
            height: Math.max(titleRow.implicitHeight, metricText.implicitHeight)

            Row {
                id: titleRow
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    color: Theme.muted
                    font.pixelSize: 10
                    font.letterSpacing: 1.3
                    font.capitalization: Font.AllUppercase
                }

                HelpHint {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.helpText.length > 0
                    text: root.helpText
                }
            }

            Text {
                id: metricText
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: root.metric
                color: root.metricColor
                font.pixelSize: 15
                font.bold: true
                font.capitalization: Font.AllUppercase
            }
        }

        Rectangle {
            visible: root.progress >= 0
            width: parent.width
            height: 8
            radius: 2
            color: Theme.panel2
            border.color: Theme.line
            border.width: 1
            clip: true

            Rectangle {
                width: parent.width * Math.max(0, Math.min(100, root.progress)) / 100
                height: parent.height
                color: Theme.accent

                Behavior on width {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }
            }
        }

        Item { width: 1; height: 8 }

        Column {
            id: contentColumn
            width: parent.width
            spacing: 2
        }
    }
}
