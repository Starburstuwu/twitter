import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    property string headerText
    property string descriptionText
    property string yesButtonText
    property string noButtonText

    property var yesButtonFunction
    property var noButtonFunction

    expandedContent: ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.topMargin: 16
        anchors.rightMargin: 16
        anchors.leftMargin: 16

        spacing: 8

        onImplicitHeightChanged: {
            root.expandedHeight = content.implicitHeight + 32
        }

        Header2TextType {
            Layout.fillWidth: true

            text: headerText
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8

            text: descriptionText
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16

            text: yesButtonText

            onClicked: {
                if (yesButtonFunction && typeof yesButtonFunction === "function") {
                    yesButtonFunction()
                }
            }
        }

        BasicButtonType {
            Layout.fillWidth: true

            defaultColor: "transparent"
            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
            pressedColor: Qt.rgba(1, 1, 1, 0.12)
            disabledColor: "#878B91"
            textColor: "#D7D8DB"
            borderWidth: 1

            text: noButtonText

            onClicked: {
                if (noButtonFunction && typeof noButtonFunction === "function") {
                    noButtonFunction()
                }
            }
        }
    }
}
