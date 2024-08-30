import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    readonly property string drawerExpandedStateName: "expanded"
    readonly property string drawerCollapsedStateName: "collapsed"

    readonly property bool isExpandedStateActive: drawerContent.state === root.drawerExpandedStateName
    readonly property bool isCollapsedStateActive: drawerContent.state === root.drawerCollapsedStateName

    readonly property bool isOpened: root.isExpandedStateActive || (isCollapsedStateActive && dragArea.drag.active === true)
    readonly property bool isClosed: root.isCollapsedStateActive && (dragArea.drag.active === false)

    property Component collapsedStateContent
    property Component expandedStateContent

    property string defaultColor: AmneziaStyle.color.onyxBlack
    property string borderColor: AmneziaStyle.color.slateGray

    property real expandedHeight
    property real collapsedHeight: 0

    property int depthIndex: 0

    signal cursorEntered
    signal cursorExited
    signal pressed(bool pressed, bool entered)

    signal aboutToHide
    signal aboutToShow
    signal close
    signal open
    signal closed
    signal opened

    property bool isFocusable: true
    
    // Keys.onTabPressed: {
    //     console.debug("--> Tab is pressed on ", objectName)
    //     FocusController.nextKeyTabItem()
    // }

    Connections {
        target: PageController

        function onCloseTopDrawer() {
            if (depthIndex === PageController.getDrawerDepth()) {
                if (isCollapsedStateActive) {
                    return
                }

                aboutToHide()

                drawerContent.state = root.drawerCollapsedStateName
                depthIndex = 0
                closed()
            }
        }
    }

    Connections {
        target: root

        function onClose() {
            if (isCollapsedStateActive) {
                return
            }

            aboutToHide()

            drawerContent.state = root.drawerCollapsedStateName
            depthIndex = 0
            PageController.setDrawerDepth(PageController.getDrawerDepth() - 1)
            closed()
        }

        function onOpen() {
            if (isExpandedStateActive) {
                return
            }

            aboutToShow()

            drawerContent.state = root.drawerExpandedStateName
            depthIndex = PageController.getDrawerDepth() + 1
            PageController.setDrawerDepth(depthIndex)
            opened()
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: root.isCollapsedStateActive ? AmneziaStyle.color.transparent : Qt.rgba(14/255, 14/255, 17/255, 0.8)

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        id: emptyArea
        anchors.fill: parent
        enabled: root.isExpandedStateActive
        visible: enabled
        onClicked: {
            root.close()
        }
    }

    MouseArea {
        id: dragArea

        anchors.fill: drawerContentBackground
        cursorShape: root.isCollapsedStateActive ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true

        enabled: drawerContent.implicitHeight > 0

        drag.target: drawerContent
        drag.axis: Drag.YAxis
        drag.maximumY: root.height - root.collapsedHeight
        drag.minimumY: root.height - root.expandedHeight

        /** If drag area is released at any point other than min or max y, transition to the other state */
        onReleased: {
            if (root.isCollapsedStateActive && drawerContent.y < dragArea.drag.maximumY) {
                root.open()
                return
            }
            if (root.isExpandedStateActive && drawerContent.y > dragArea.drag.minimumY) {
                root.close()
                return
            }
        }

        onEntered: {
            console.log("===>> dragArea cursor entered")
            root.cursorEntered()
        }
        onExited: {
            console.log("===>> dragArea cursor exited")
            root.cursorExited()
        }
        onPressedChanged: {
            root.pressed(pressed, entered)
        }

        onClicked: {
            if (root.isCollapsedStateActive) {
                root.open()
            }
        }
    }

    Rectangle {
        id: drawerContentBackground

        anchors { left: drawerContent.left; right: drawerContent.right; top: drawerContent.top }
        height: root.height
        radius: 16
        color: root.defaultColor
        border.color: root.borderColor
        border.width: 1

        Rectangle {
            width: parent.radius
            height: parent.radius
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            color: parent.color
        }
    }

    Item {
        id: drawerContent

        Drag.active: dragArea.drag.active
        anchors.right: root.right
        anchors.left: root.left
        y: root.height - drawerContent.height
        state: root.drawerCollapsedStateName

        implicitHeight: root.isCollapsedStateActive ? collapsedHeight : expandedHeight

        onStateChanged: {
            if (root.isCollapsedStateActive) {
                var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
                if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(initialPageNavigationBarColor)
                }
                return
            }
            if (root.isExpandedStateActive) {
                if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(0xFF1C1D21)
                }
                return
            }
        }

        states: [
            State {
                name: root.drawerCollapsedStateName
                PropertyChanges {
                    target: drawerContent
                    y: root.height - root.collapsedHeight
                }
            },
            State {
                name: root.drawerExpandedStateName
                PropertyChanges {
                    target: drawerContent
                    y: dragArea.drag.minimumY

                }
            }
        ]

        transitions: [
            Transition {
                from: root.drawerCollapsedStateName
                to: root.drawerExpandedStateName
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            },
            Transition {
                from: root.drawerExpandedStateName
                to: root.drawerCollapsedStateName
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            }
        ]

        Loader {
            id: collapsedLoader

            sourceComponent: root.collapsedStateContent

            anchors.right: parent.right
            anchors.left: parent.left
        }

        Loader {
            id: expandedLoader

            visible: root.isExpandedStateActive
            sourceComponent: root.expandedStateContent

            anchors.right: parent.right
            anchors.left: parent.left
        }
    }
}
