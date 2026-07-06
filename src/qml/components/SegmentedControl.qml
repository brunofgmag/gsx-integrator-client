import QtQuick
import QtQuick.Controls.Basic

Row {
    id: root

    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    spacing: 6

    Repeater {
        model: root.model

        Button {
            id: segment

            required property int index
            required property string modelData
            readonly property bool selected: root.currentIndex === index

            implicitHeight: 26
            leftPadding: 12
            rightPadding: 12

            contentItem: Text {
                text: segment.modelData
                color: segment.selected ? Theme.accentText : Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: Theme.radiusSmall
                color: segment.selected ? Theme.accent
                                        : (segment.hovered ? Theme.panel2 : "transparent")
                border.color: segment.selected ? Theme.accent : Theme.line
                border.width: 1
            }

            onClicked: root.activated(segment.index)
        }
    }
}
