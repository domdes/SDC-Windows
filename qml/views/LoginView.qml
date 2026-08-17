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
            width: Math.min(parent.width - 40, 420)
            height: 320
            radius: 20
            color: "#FFFFFF"
            anchors.centerIn: parent

            // Subtle Drop Shadow
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: 4
                radius: 20
                color: "#000000"
                opacity: 0.05
                z: -1
            }

            Column {
                anchors.centerIn: parent
                spacing: 14
                width: parent.width - 48

                // Logo Asyuhada
                Image {
                    source: "qrc:/qml/assets/logo_asyuhada.png"
                    width: 64
                    height: 64
                    fillMode: Image.PreserveAspectFit
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "Selamat Datang"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#065F46"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "Portal Yayasan Asyuhada Jaya Bekasi"
                    font.pixelSize: 14
                    color: "#6B7280"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Item { width: 1; height: 10 }

                // Google Login Button (Aesthetic, White Pill with Google Logo)
                Button {
                    id: googleLoginBtn
                    width: parent.width
                    height: 50
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: appViewModel.startGoogleLogin()

                    hoverEnabled: true

                    background: Rectangle {
                        color: googleLoginBtn.pressed ? "#F3F4F6" : (googleLoginBtn.hovered ? "#F9FAFB" : "#FFFFFF")
                        border.color: googleLoginBtn.hovered ? "#9CA3AF" : "#D1D5DB"
                        border.width: 1.5
                        radius: 25

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Row {
                        anchors.centerIn: parent
                        spacing: 12

                        Image {
                            source: "qrc:/qml/assets/google_logo.png"
                            width: 22
                            height: 22
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            smooth: true
                        }

                        Text {
                            text: "Login dengan Google"
                            font.pixelSize: 15
                            font.bold: true
                            color: "#1F2937"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: appViewModel.startGoogleLogin()
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
                width: Math.min(parent.width - 40, 500)

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
