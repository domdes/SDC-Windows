import QtQuick
import QtQuick.Controls

Rectangle {
    id: cardRoot
    width: parent.width
    height: 72
    radius: 16

    property string profileTitle: "Daerah Bekasi Selatan"
    property string hostText: "993.kanzul-mubaarok.org"
    property string portText: "64738"
    signal clicked()

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1E3A8A" }
        GradientStop { position: 1.0; color: "#1D4ED8" }
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 14

        Rectangle {
            width: 44
            height: 44
            radius: 22
            color: "#33FFFFFF"

            Text {
                text: "🎧"
                font.pixelSize: 22
                anchors.centerIn: parent
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: cardRoot.profileTitle
                font.pixelSize: 15
                font.bold: true
                color: "#FFFFFF"
            }

            Text {
                text: cardRoot.hostText
                font.pixelSize: 12
                color: "#93C5FD"
            }

            Text {
                text: cardRoot.portText
                font.pixelSize: 11
                color: "#BFDBFE"
            }
        }
    }

    Text {
        text: "➜"
        font.pixelSize: 22
        color: "#FFFFFF"
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
    }

    MouseArea {
        anchors.fill: parent
        onClicked: cardRoot.clicked()
    }
}
