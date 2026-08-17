import QtQuick
import QtQuick.Controls
import "../components"

Item {
    id: profileListPage

    Rectangle {
        anchors.fill: parent
        color: "#EAE7DC"

        HeaderBar {
            id: headerBar
            titleText: "SENKOM Digital Communication"
            showGear: true
            showLogout: true
            showLog: false
            onLogClicked: mainWindow.toggleLogPanel()
            onGearClicked: settingsModal.open()
            onLogoutClicked: appViewModel.logout()
        }

        ScrollView {
            anchors.top: headerBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            contentWidth: availableWidth
            clip: true

            Column {
                width: parent.width
                spacing: 14
                topPadding: 16
                bottomPadding: 32

                // Active Logged In User Info Card (Roomy, strictly 90% window width, centered)
                Rectangle {
                    width: parent.width * 0.9
                    height: 72
                    radius: 14
                    color: "#1E293B"
                    anchors.horizontalCenter: parent.horizontalCenter

                    Row {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            width: 44
                            height: 44
                            radius: 22
                            color: "#047857"
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "👤"
                                font.pixelSize: 22
                                anchors.centerIn: parent
                            }
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4
                            width: parent.width - 64

                            Row {
                                spacing: 8
                                width: parent.width

                                Text {
                                    text: appViewModel.activeMumbleUsername !== "" ? appViewModel.activeMumbleUsername : appViewModel.activeUserName
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: "#FFFFFF"
                                    elide: Text.ElideRight
                                    width: Math.max(100, parent.width - roleBadge.width - 12)
                                }

                                Rectangle {
                                    id: roleBadge
                                    color: "#047857"
                                    radius: 8
                                    width: roleText.width + 14
                                    height: roleText.height + 4
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        id: roleText
                                        text: appViewModel.activeUserRole.toUpperCase()
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: "#FFFFFF"
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            Text {
                                text: appViewModel.activeUserEmail
                                font.pixelSize: 12
                                color: "#9CA3AF"
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                    }
                }

                // Section Header: PROFIL UTAMA (PRE-INSTALLED)
                Item {
                    width: parent.width * 0.9
                    height: 20
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "PROFIL UTAMA (PRE-INSTALLED)"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#6B7280"
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Preinstalled Profile Cards (Strictly 90% window width, centered)
                Repeater {
                    model: appViewModel.preinstalledProfiles
                    ProfileCard {
                        width: parent.width * 0.9
                        anchors.horizontalCenter: parent.horizontalCenter
                        profileTitle: modelData.name
                        hostText: modelData.host
                        portText: String(modelData.port)
                        onClicked: appViewModel.connectProfileById(modelData.id)
                    }
                }
            }
        }
    }

    SettingsModal {
        id: settingsModal
    }
}
