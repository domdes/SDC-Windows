import QtQuick
import QtQuick.Controls

Item {
    id: splashPage
    Rectangle {
        anchors.fill: parent
        color: "#0F172A"

        Column {
            anchors.centerIn: parent
            spacing: 20

            Rectangle {
                width: 90
                height: 90
                radius: 45
                color: "#047857"
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text: "🎙️"
                    font.pixelSize: 44
                    anchors.centerIn: parent
                }
            }

            Text {
                text: "SDC YAJB"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Yayasan Asyuhada Jaya Bekasi"
                font.pixelSize: 14
                font.bold: true
                color: "#34D399"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            BusyIndicator {
                running: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
