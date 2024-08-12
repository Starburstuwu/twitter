import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 0

        Image {
            id: image
            source: "qrc:/images/amneziaBigLogo.png"

            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.topMargin: 32
            Layout.preferredWidth: 360
            Layout.preferredHeight: 287
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Free service for creating a personal VPN on your server.") +
                  qsTr(" Helps you access blocked content without revealing your privacy, even to VPN providers.")
        }

        Item {
            id: focusItem
            KeyNavigation.tab: startButton
            Layout.fillHeight: true
        }

        BasicButtonType {
            id: startButton
            Layout.fillWidth: true
            Layout.bottomMargin: 48
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Let's get started")

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
            }

            Keys.onTabPressed: lastItemTabClicked(focusItem)
        }
    }
}
