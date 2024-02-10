import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: "Amnezia DNS"
                descriptionText: qsTr("A DNS service is installed on your server, and it is only accessible via VPN.\n") +
                                 qsTr("The DNS address is the same as the address of your server. You can configure DNS in the settings, under the connections tab.")
            }

            LabelWithButtonType {
                id: removeButton

                Layout.topMargin: 24
                width: parent.width

                text: qsTr("Remove ") + ContainersModel.getCurrentlyProcessedContainerName()
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getCurrentlyProcessedContainerName())
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeCurrentlyProcessedContainer()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {}

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}
