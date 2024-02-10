import QtQuick 2.12
import QtQuick.Controls 2.12
import "../Config"

Flickable {
    id: fl

    clip: true
    width: parent.width

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.rightMargin: 1

    Keys.onUpPressed: scrollBar.decrease()
    Keys.onDownPressed: scrollBar.increase()

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        policy: fl.height >= fl.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
    }
}
