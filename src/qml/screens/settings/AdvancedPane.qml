pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var settingsVm

    readonly property var rendererIds: ["software", "d3d12", "opengl"]
    readonly property var rendererLabels: ({
        "d3d12": "D3D12", "opengl": "OpenGL", "vulkan": "Vulkan",
        "software": "Software", "d3d11": "D3D11"
    })

    readonly property bool restartPending: root.settingsVm.activeRenderer.length > 0
                                        && root.settingsVm.activeRenderer !== root.settingsVm.renderer

    spacing: 8

    SettingRow {
        Layout.fillWidth: true
        title: qsTr("Renderer")
        caption: qsTr("Graphics Backend")
        helpText: qsTr("Software is the default because it uses the least RAM and VRAM. D3D12 draws on the graphics card; OpenGL is there for drivers that misbehave.")

        SegmentedControl {
            anchors.verticalCenter: parent.verticalCenter
            model: ["Software", "D3D12", "OpenGL"]
            currentIndex: Math.max(0, root.rendererIds.indexOf(root.settingsVm.renderer))
            onActivated: index => root.settingsVm.renderer = root.rendererIds[index]
        }
    }

    Advisory {
        Layout.fillWidth: true
        text: root.restartPending
              ? qsTr("Restart GSX Integrator to draw with %1. It is still using %2.")
                    .arg(root.rendererLabels[root.settingsVm.renderer] ?? root.settingsVm.renderer)
                    .arg(root.rendererLabels[root.settingsVm.activeRenderer] ?? root.settingsVm.activeRenderer)
              : ""
    }

    Item {
        Layout.fillHeight: true
    }
}
