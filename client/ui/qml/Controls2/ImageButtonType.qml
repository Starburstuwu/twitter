import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property string image

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.08)
    property string defaultColor: "transparent"
    property string pressedColor: Qt.rgba(1, 1, 1, 0.12)
    property string disableColor: "#2C2D30"

    property string imageColor: "#878B91"
    property string disableImageColor: "#2C2D30"

    property alias backgroundColor: background.color
    property alias backgroundRadius: background.radius

    hoverEnabled: true

    icon.source: image
    icon.color: root.enabled ? imageColor : disableImageColor

    Behavior on icon.color {
        PropertyAnimation { duration: 200 }
    }

    background: Rectangle {
        id: background

        anchors.fill: parent
        color: {
            if (root.enabled) {
                if (root.pressed) {
                    return pressedColor
                }
                return hovered ? hoveredColor : defaultColor
            }
            return defaultColor
        }
        radius: 12
        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }
}
