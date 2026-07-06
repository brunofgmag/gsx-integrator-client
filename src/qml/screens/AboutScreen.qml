import QtQuick

Item {
    id: root

    property real viewportHeight: 0

    implicitHeight: Math.max(col.implicitHeight, viewportHeight - 32)

    Column {
        id: col
        y: (root.implicitHeight - implicitHeight) / 2
        width: parent.width
        spacing: 10

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/icons/app-icon_128.png"
            sourceSize: Qt.size(64, 64)
        }

        Text {
            width: parent.width
            topPadding: 8
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("GSX Integrator")
            color: Theme.text
            font.pixelSize: 22
            font.bold: true
            font.letterSpacing: 2.5
            font.capitalization: Font.AllUppercase
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Ver %1 · MSFS 2020 / 2024 · GSX Pro").arg(Qt.application.version)
            color: Theme.muted
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(400, parent.width)
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Automates the full GSX turnaround.")
            color: Theme.muted
            font.pixelSize: 11
            font.letterSpacing: 0.5
            font.capitalization: Font.AllUppercase
            lineHeight: 1.5
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            topPadding: 18
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("© 2026 · Not affiliated with FSDreamTeam or Microsoft")
            color: Theme.faint
            font.pixelSize: 10
            font.letterSpacing: 1.0
            font.capitalization: Font.AllUppercase
        }
    }
}
