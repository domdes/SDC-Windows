import QtQuick
import QtQuick.Controls

Rectangle {
    id: headerRoot
    width: parent.width
    height: 56
    color: "#047857"

    property string titleText: "SENKOM Digital Communication"
    property string statusText: ""
    property bool showGear: false
    property bool showLogout: false
    property bool showBack: false
    property bool showLog: true
    property bool showAudioIcons: false

    signal backClicked()
    signal gearClicked()
    signal logoutClicked()
    signal logClicked()

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Rectangle {
            width: 32
            height: 32
            radius: 16
            color: "transparent"
            visible: headerRoot.showBack

            Text {
                text: "◀"
                font.pixelSize: 18
                color: "#FFFFFF"
                anchors.centerIn: parent
            }

            MouseArea {
                anchors.fill: parent
                onClicked: headerRoot.backClicked()
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            Text {
                text: headerRoot.titleText
                font.pixelSize: 16
                font.bold: true
                color: "#FFFFFF"
            }

            Text {
                text: headerRoot.statusText
                font.pixelSize: 11
                color: "#A7F3D0"
                visible: headerRoot.statusText !== ""
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Row {
            spacing: 8
            visible: headerRoot.showAudioIcons

            Text { text: "📶"; font.pixelSize: 16 }
            Text { text: "🎙️"; font.pixelSize: 16 }
            Text { text: "🎧"; font.pixelSize: 16 }
        }

        Text {
            text: "📋"
            font.pixelSize: 18
            color: "#FFFFFF"
            visible: headerRoot.showLog
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: headerRoot.logClicked()
            }
        }

        Text {
            text: "🔒 Logout"
            font.pixelSize: 12
            color: "#FFFFFF"
            visible: headerRoot.showLogout
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: headerRoot.logoutClicked()
            }
        }

        Text {
            text: "⚙️"
            font.pixelSize: 20
            color: "#FFFFFF"
            visible: headerRoot.showGear
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: headerRoot.gearClicked()
            }
        }
    }
}
