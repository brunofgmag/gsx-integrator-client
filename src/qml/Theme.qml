pragma Singleton
import QtQuick

QtObject {
    property bool dark: true

    readonly property color bg: dark ? "#0b1016" : "#e7ebef"
    readonly property color panel: dark ? "#121a24" : "#f7f9fb"
    readonly property color panel2: dark ? "#0d141c" : "#e0e6ec"
    readonly property color line: dark ? "#1e2c3d" : "#c8d2dc"

    readonly property color text: dark ? "#e6eef7" : "#17222e"
    readonly property color muted: dark ? "#7e91a6" : "#5c6e80"
    readonly property color faint: dark ? "#54677c" : "#8496a8"

    readonly property color accent: dark ? "#33bfff" : "#0072c3"
    readonly property color accentText: dark ? "#04121f" : "#ffffff"

    readonly property color ok: dark ? "#35d07f" : "#0a7d4b"
    readonly property color amber: dark ? "#ffb454" : "#8a5a00"
    readonly property color red: dark ? "#ff5d5d" : "#c43030"

    readonly property color tipBg: dark ? "#231a09" : "#f3ead2"
    readonly property color tipFg: dark ? "#ffce8a" : "#6e4a00"

    readonly property color tooltipBg: dark ? "#1c2836" : "#17222e"
    readonly property color tooltipFg: dark ? "#e6eef7" : "#eef4fa"
    readonly property color tooltipBorder: dark ? "#31445c" : "#17222e"

    readonly property int radius: 4
    readonly property int radiusSmall: 3

    readonly property string monoFamily: "Cascadia Mono"
}
