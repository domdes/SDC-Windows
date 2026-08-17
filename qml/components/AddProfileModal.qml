import QtQuick
import QtQuick.Controls

Popup {
    id: addProfilePopup
    width: 360
    height: 400
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true

    Overlay.modal: Rectangle {
        color: "#8C000000"
    }

    background: Rectangle {
        color: "#FFFFFF"
        radius: 16
        border.color: "#E5E7EB"
        border.width: 1
    }

    contentItem: Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        Text {
            text: "Tambah Profil Baru"
            font.pixelSize: 18
            font.bold: true
            color: "#111827"
        }

        TextField {
            id: txtName
            width: parent.width
            height: 40
            placeholderText: "Nama Profil"
            font.pixelSize: 13
            selectByMouse: true
        }

        TextField {
            id: txtHost
            width: parent.width
            height: 40
            placeholderText: "Host / IP Server"
            font.pixelSize: 13
            selectByMouse: true
        }

        TextField {
            id: txtPort
            width: parent.width
            height: 40
            text: "64738"
            placeholderText: "Port Server"
            font.pixelSize: 13
            selectByMouse: true
        }

        TextField {
            id: txtPassword
            width: parent.width
            height: 40
            placeholderText: "Password (Opsional)"
            echoMode: TextInput.Password
            font.pixelSize: 13
            selectByMouse: true
        }

        Item { width: 1; height: 8 }

        Row {
            anchors.right: parent.right
            spacing: 12

            Button {
                width: 100
                height: 38

                background: Rectangle {
                    color: cancelHover.containsMouse ? "#E5E7EB" : "#F3F4F6"
                    radius: 8
                    border.color: "#D1D5DB"
                    border.width: 1
                }

                MouseArea {
                    id: cancelHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: addProfilePopup.close()
                }

                contentItem: Text {
                    text: "Batal"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#374151"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                width: 130
                height: 38

                background: Rectangle {
                    color: saveHover.containsMouse ? "#047857" : "#059669"
                    radius: 8
                }

                MouseArea {
                    id: saveHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (txtHost.text.trim() !== "") {
                            appViewModel.connectProfile(txtHost.text.trim(), parseInt(txtPort.text.trim()) || 64738)
                        }
                        addProfilePopup.close()
                    }
                }

                contentItem: Text {
                    text: "Hubungkan"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
