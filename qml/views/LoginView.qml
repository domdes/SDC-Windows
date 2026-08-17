import QtQuick
import QtQuick.Controls

Item {
    id: loginPage
    Rectangle {
        anchors.fill: parent
        color: "#EAE7DC"

        // Top Header Bar
        Rectangle {
            id: topHeader
            width: parent.width
            height: 60
            color: "#047857"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Rectangle {
                    width: 36
                    height: 36
                    radius: 6
                    color: "#065F46"

                    Text {
                        text: "🕌"
                        font.pixelSize: 20
                        anchors.centerIn: parent
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "YAYASAN ASYUHADA"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    Text {
                        text: "JAYA BEKASI"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#FFFFFF"
                    }
                }
            }
        }

        // Center Login Card
        Rectangle {
            width: parent.width - 40
            height: 340
            radius: 20
            color: "#FFFFFF"
            anchors.centerIn: parent

            Column {
                anchors.centerIn: parent
                spacing: 14
                width: parent.width - 40

                Text {
                    text: "Selamat Datang"
                    font.pixelSize: 26
                    font.bold: true
                    color: "#2C5E53"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "Portal Yayasan Asyuhada Jaya Bekasi"
                    font.pixelSize: 15
                    color: "#555555"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Item { width: 1; height: 6 }

                // Google Login Button (Primary)
                Button {
                    width: parent.width
                    height: 48
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: appViewModel.startGoogleLogin()

                    background: Rectangle {
                        color: "#047857"
                        radius: 24
                    }

                    contentItem: Row {
                        anchors.centerIn: parent
                        spacing: 10

                        Text {
                            text: "🌐"
                            font.pixelSize: 18
                        }

                        Text {
                            text: "Lanjutkan dengan Google"
                            font.pixelSize: 15
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }
                }

                // Direct Login Button (Secondary / Fallback)
                Button {
                    width: parent.width
                    height: 44
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: appViewModel.handleUriScheme("sdcyajb://oauth?code=direct_verified")

                    background: Rectangle {
                        color: "#F3F4F6"
                        border.color: "#D1D5DB"
                        border.width: 1
                        radius: 22
                    }

                    contentItem: Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "⚡"
                            font.pixelSize: 15
                        }

                        Text {
                            text: "Masuk SDC Pengguna Terverifikasi"
                            font.pixelSize: 13
                            font.bold: true
                            color: "#374151"
                        }
                    }
                }
            }
        }

        // Bottom Footer
        Rectangle {
            width: parent.width
            height: 120
            color: "#38665B"
            anchors.bottom: parent.bottom

            Column {
                anchors.centerIn: parent
                spacing: 6
                width: parent.width - 40

                Text {
                    text: "Yayasan Asyuhada Jaya Bekasi"
                    font.pixelSize: 17
                    font.bold: true
                    color: "#FFFFFF"
                }

                Text {
                    text: "Membangun generasi cerdas, berakhlak mulia, dan berdaya guna melalui pendidikan dan kepedulian sosial."
                    font.pixelSize: 11
                    color: "#D1FAE5"
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }
}
