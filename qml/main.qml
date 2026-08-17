import QtQuick
import QtQuick.Window
import QtQuick.Controls
import "views"
import "components"

Window {
    id: mainWindow
    width: 440
    height: 780
    visible: true
    title: "SDC YAJB - Yayasan Asyuhada Jaya Bekasi"
    color: "#0F172A"

    property bool isLogPanelOpen: false

    function toggleLogPanel() {
        isLogPanelOpen = !isLogPanelOpen
    }

    function openLogPanel() {
        isLogPanelOpen = true
    }

    // 1. Main View Area (Anchored to top and sits above the log panel)
    Item {
        id: mainViewContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: logBottomPanel.visible ? logBottomPanel.top : parent.bottom

        StackView {
            id: stackView
            anchors.fill: parent
            initialItem: splashView
        }
    }

    // 2. Persistent Bottom Log Panel (Docked at the bottom of the main window)
    Rectangle {
        id: logBottomPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: mainWindow.isLogPanelOpen ? 240 : 34
        color: "#0F172A"
        visible: appViewModel.currentView !== "SplashView"
        border.color: "#334155"
        border.width: 1
        clip: true

        Behavior on height {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }

        // Top Strip / Header of Log Panel
        Rectangle {
            id: logHeaderStrip
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 34
            color: logHeaderHover.containsMouse ? "#1E293B" : "#0F172A"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: "📋"
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "Log Server Mumble"
                    font.pixelSize: 12
                    font.bold: true
                    color: mainWindow.isLogPanelOpen ? "#38BDF8" : "#E2E8F0"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: voiceViewModel.isConnected ? "#34D399" : (voiceViewModel.isConnecting ? "#60A5FA" : "#94A3B8")
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: voiceViewModel.statusText
                    font.pixelSize: 10
                    color: "#94A3B8"
                    elide: Text.ElideRight
                    width: Math.max(60, logHeaderStrip.width - (mainWindow.isLogPanelOpen ? 300 : 230))
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                // Clear Button (Visible when open)
                Rectangle {
                    width: 65
                    height: 22
                    radius: 4
                    color: clearLogBtnHover.containsMouse ? "#334155" : "#1E293B"
                    visible: mainWindow.isLogPanelOpen
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: "Bersihkan"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#94A3B8"
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: clearLogBtnHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceViewModel.clearDebugLogs()
                    }
                }

                // Toggle Button
                Rectangle {
                    width: mainWindow.isLogPanelOpen ? 90 : 105
                    height: 24
                    radius: 4
                    color: toggleBtnHover.containsMouse ? "#0284C7" : "#0369A1"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: mainWindow.isLogPanelOpen ? "▼ Sembunyikan" : "▲ Buka Log Server"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#FFFFFF"
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: toggleBtnHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mainWindow.toggleLogPanel()
                    }
                }
            }

            MouseArea {
                id: logHeaderHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: mainWindow.toggleLogPanel()
            }
        }

        // Terminal Viewport
        Rectangle {
            anchors.top: logHeaderStrip.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#020617"
            visible: mainWindow.isLogPanelOpen
            clip: true

            ScrollView {
                id: logScrollView
                anchors.fill: parent
                anchors.margins: 6
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                TextArea {
                    id: logTextArea
                    text: voiceViewModel.debugLogs !== "" ? voiceViewModel.debugLogs : "[INFO] Menunggu aktivitas koneksi dan return server Mumble..."
                    font.family: "Consolas, 'Courier New', monospace"
                    font.pixelSize: 11
                    color: "#4ADE80"
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    background: null
                    selectByMouse: true

                    onTextChanged: {
                        logScrollView.ScrollBar.vertical.position = 1.0
                    }
                }
            }
        }
    }

    Component {
        id: splashView
        SplashView {}
    }

    Component {
        id: loginView
        LoginView {}
    }

    Component {
        id: profileListView
        ProfileListView {}
    }

    Component {
        id: connectedView
        ConnectedView {}
    }

    function updateCurrentView() {
        if (appViewModel.currentView === "SplashView") {
            stackView.replace(splashView)
        } else if (appViewModel.currentView === "LoginView") {
            stackView.replace(loginView)
        } else if (appViewModel.currentView === "ProfileListView") {
            stackView.replace(profileListView)
        } else if (appViewModel.currentView === "ConnectedView") {
            stackView.replace(connectedView)
        }
    }

    Component.onCompleted: {
        updateCurrentView()
    }

    Connections {
        target: appViewModel
        function onCurrentViewChanged() {
            updateCurrentView()
        }
    }
}
