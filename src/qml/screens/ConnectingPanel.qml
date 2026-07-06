import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    spacing: 14

    Item {
        Layout.fillHeight: true
    }

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: linkColumn.implicitWidth + 68
        implicitHeight: linkColumn.implicitHeight + 40
        radius: Theme.radius
        color: Theme.panel
        border.color: Theme.line
        border.width: 1

        Column {
            id: linkColumn
            anchors.centerIn: parent
            spacing: 7

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Simulator link")
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 1.6
                font.capitalization: Font.AllUppercase
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("No link")
                color: Theme.amber
                font.pixelSize: 30
                font.bold: true
                font.letterSpacing: 3
                font.capitalization: Font.AllUppercase

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: 0.3; duration: 900; easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: 1.0; duration: 900; easing.type: Easing.InOutSine
                    }
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Start MSFS and load a flight. The link is automatic.")
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 0.5
                font.capitalization: Font.AllUppercase
            }
        }
    }

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(420, root.width)
        implicitHeight: checklist.implicitHeight
        radius: Theme.radius
        color: Theme.panel
        border.color: Theme.line
        border.width: 1

        Column {
            id: checklist
            width: parent.width

            Repeater {
                model: [
                    {label: qsTr("SimConnect"), help: "", searching: true},
                    {label: qsTr("GSX Pro"), help: "", searching: false},
                    {label: qsTr("Supported Aircraft"), help: "", searching: false}
                ]

                Item {
                    id: checkRow

                    required property var modelData
                    required property int index

                    width: checklist.width
                    height: 38

                    Rectangle {
                        anchors.top: parent.top
                        width: parent.width
                        height: 1
                        color: Theme.line
                        visible: checkRow.index > 0
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 7

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: checkRow.modelData.label
                            color: Theme.text
                            font.pixelSize: 10
                            font.letterSpacing: 1.0
                            font.capitalization: Font.AllUppercase
                        }

                        HelpHint {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: checkRow.modelData.help.length > 0
                            text: checkRow.modelData.help
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: checkRow.modelData.searching ? qsTr("Searching") : qsTr("Standby")
                        color: checkRow.modelData.searching ? Theme.amber : Theme.muted
                        font.pixelSize: 10
                        font.bold: checkRow.modelData.searching
                        font.capitalization: Font.AllUppercase

                        SequentialAnimation on opacity {
                            running: checkRow.modelData.searching
                            loops: Animation.Infinite
                            NumberAnimation {
                                to: 0.3; duration: 800; easing.type: Easing.InOutSine
                            }
                            NumberAnimation {
                                to: 1.0; duration: 800; easing.type: Easing.InOutSine
                            }
                        }
                    }
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
