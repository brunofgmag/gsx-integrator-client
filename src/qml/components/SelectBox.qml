import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: root

    implicitWidth: 170
    implicitHeight: 26
    leftPadding: 10
    rightPadding: 26

    font.pixelSize: 10
    font.letterSpacing: 1.0
    font.capitalization: Font.AllUppercase

    contentItem: Text {
        text: root.displayText
        color: Theme.text
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: root.width - width - 9
        y: root.topPadding + (root.availableHeight - height) / 2
        text: "▾"
        color: root.pressed || root.popup.visible ? Theme.accent : Theme.muted
        font.pixelSize: 10
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.hovered ? Theme.panel2 : "transparent"
        border.color: root.activeFocus || root.popup.visible ? Theme.accent : Theme.line
        border.width: 1
    }

    delegate: ItemDelegate {
        id: option

        required property int index
        required property string modelData

        width: root.width
        height: 26
        padding: 0

        contentItem: Text {
            leftPadding: 10
            text: option.modelData
            color: option.index === root.currentIndex ? Theme.accentText : Theme.text
            font: root.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: option.index === root.currentIndex ? Theme.accent
                                                      : (option.hovered ? Theme.panel2 : "transparent")
        }
    }

    popup: Popup {
        y: root.height + 3
        width: root.width
        implicitHeight: contentItem.implicitHeight + 8
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
        }

        background: Rectangle {
            radius: Theme.radiusSmall
            color: Theme.panel
            border.color: Theme.line
            border.width: 1
        }
    }
}
