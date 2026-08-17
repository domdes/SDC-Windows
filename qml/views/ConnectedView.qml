import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: connectedViewRoot

    Connections {
        target: voiceViewModel
        function onPermissionDeniedAlert(alertMsg) {
            permissionAlert.textMessage = alertMsg
            permissionAlert.open()
        }
        function onConnectionErrorOccurred(errorMsg) {
            errorAlert.textMessage = errorMsg
            errorAlert.open()
        }
    }

    // Polished Permission Denied Alert Popup
    Popup {
        id: permissionAlert
        width: 320
        height: 200
        anchors.centerIn: Overlay.overlay
        modal: true
        focus: true
        property string textMessage: ""

        Overlay.modal: Rectangle {
            color: "#8C000000"
        }

        background: Rectangle {
            color: "#FFFFFF"
            radius: 16
            border.color: "#FCA5A5"
            border.width: 1
        }

        contentItem: Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            Row {
                spacing: 8
                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: "#FEE2E2"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: "⚠️"
                        font.pixelSize: 14
                        anchors.centerIn: parent
                    }
                }
                Text {
                    text: "Akses Ditolak"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#DC2626"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                text: permissionAlert.textMessage !== "" ? permissionAlert.textMessage : "Anda tidak memiliki token akses yang sesuai untuk channel ini."
                font.pixelSize: 12
                color: "#4B5563"
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }

            Button {
                width: parent.width
                height: 38
                anchors.horizontalCenter: parent.horizontalCenter

                background: Rectangle {
                    color: okPermHover.containsMouse ? "#DC2626" : "#EF4444"
                    radius: 8
                }

                MouseArea {
                    id: okPermHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: permissionAlert.close()
                }

                contentItem: Text {
                    text: "OK"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Connection Error Alert Popup
    Popup {
        id: errorAlert
        width: 320
        height: 200
        anchors.centerIn: Overlay.overlay
        modal: true
        focus: true
        property string textMessage: ""

        Overlay.modal: Rectangle {
            color: "#8C000000"
        }

        background: Rectangle {
            color: "#FFFFFF"
            radius: 16
            border.color: "#FCA5A5"
            border.width: 1
        }

        contentItem: Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            Row {
                spacing: 8
                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: "#FEE2E2"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: "⚠️"
                        font.pixelSize: 14
                        anchors.centerIn: parent
                    }
                }
                Text {
                    text: "Kesalahan Koneksi"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#DC2626"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                text: errorAlert.textMessage !== "" ? errorAlert.textMessage : "Gagal terhubung ke server."
                font.pixelSize: 12
                color: "#4B5563"
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }

            Button {
                width: parent.width
                height: 38
                anchors.horizontalCenter: parent.horizontalCenter

                background: Rectangle {
                    color: okErrHover.containsMouse ? "#DC2626" : "#EF4444"
                    radius: 8
                }

                MouseArea {
                    id: okErrHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        errorAlert.close()
                        appViewModel.disconnectAndReturnToProfiles()
                    }
                }

                contentItem: Text {
                    text: "OK"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Custom Header Bar with Back Button (Emerald Green like in APK)
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: "#047857"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                // Back / Disconnect Button (✕)
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: backHover.containsMouse ? "#065F46" : "transparent"

                    Text {
                        text: "✕"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#FFFFFF"
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: backHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: appViewModel.disconnectAndReturnToProfiles()
                    }
                }

                Text {
                    text: "SENKOM Digital Communication"
                    font.pixelSize: 15
                    font.bold: true
                    color: "#FFFFFF"
                    Layout.fillWidth: true
                }

                // Server Log Toggle Button (📋)
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: logBtnHover.containsMouse ? "#065F46" : "transparent"

                    Text {
                        text: "📋"
                        font.pixelSize: 16
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: logBtnHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainWindow.toggleLogPanel()
                    }
                }

                // Settings Gear Button
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: gearHover.containsMouse ? "#065F46" : "transparent"

                    Text {
                        text: "⚙️"
                        font.pixelSize: 16
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: gearHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsModal.open()
                    }
                }
            }
        }

        // 2. Dynamic Status Indicator Bar
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: voiceViewModel.isConnected ? "#064E3B" : (voiceViewModel.isConnecting ? "#1E3A8A" : "#7F1D1D")

            Row {
                anchors.centerIn: parent
                spacing: 8

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: voiceViewModel.isConnected ? "#34D399" : (voiceViewModel.isConnecting ? "#60A5FA" : "#F87171")
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: voiceViewModel.statusText
                    font.pixelSize: 11
                    font.bold: true
                    color: "#FFFFFF"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // 3. Channel List / Tree View Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Background solid color matching APK
            Rectangle {
                anchors.fill: parent
                color: "#EAE7DC"
            }

            // Transparent Watermark Logo Yayasan
            Image {
                anchors.centerIn: parent
                width: Math.min(parent.width * 0.55, 240)
                height: width
                source: "../assets/logo_yayasan.png"
                fillMode: Image.PreserveAspectFit
                opacity: 0.12
                smooth: true
            }

            ScrollView {
                anchors.fill: parent
                contentWidth: availableWidth
                clip: true

                Column {
                    width: parent.width
                    spacing: 2
                    topPadding: 6
                    bottomPadding: 16

                    Repeater {
                        model: voiceViewModel.channelTree
                        ChannelRow {
                            width: parent.width
                            nodeData: modelData
                            depth: 0
                        }
                    }
                }
            }
        }

        // 4. Sticky Bottom PTT Button (Hidden for Murid)
        Rectangle {
            Layout.fillWidth: true
            height: 55
            color: voiceViewModel.isPttActive ? "#DC2626" : (voiceViewModel.isMuted ? "#9CA3AF" : "#059669")
            visible: voiceViewModel.userRole !== "murid"
            opacity: (voiceViewModel.isMuted || voiceViewModel.isLocalMuted) ? 0.6 : 1.0

            MouseArea {
                anchors.fill: parent
                enabled: !(voiceViewModel.isMuted || voiceViewModel.isLocalMuted)
                cursorShape: Qt.PointingHandCursor
                onClicked: voiceViewModel.togglePtt()
            }

            Text {
                anchors.centerIn: parent
                text: voiceViewModel.isPttActive ? "TEKAN UNTUK BERHENTI" : ((voiceViewModel.isMuted || voiceViewModel.isLocalMuted) ? "TERBUNGKAP (MUTED)" : "TEKAN UNTUK BERBICARA")
                font.pixelSize: 16
                font.bold: true
                color: "#FFFFFF"
            }
        }
    }

    // 5. Connecting Indicator Overlay
    Rectangle {
        anchors.fill: parent
        color: "#CC0F172A"
        visible: voiceViewModel.isConnecting
        z: 999

        Column {
            anchors.centerIn: parent
            spacing: 16

            BusyIndicator {
                running: voiceViewModel.isConnecting
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Menghubungkan ke Server Mumble..."
                font.pixelSize: 14
                font.bold: true
                color: "#FFFFFF"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Menginisialisasi channel dan token akses..."
                font.pixelSize: 12
                color: "#94A3B8"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    SettingsModal {
        id: settingsModal
    }
}
