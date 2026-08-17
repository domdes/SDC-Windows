import QtQuick
import QtQuick.Controls

Popup {
    id: settingsPopup
    width: 360
    height: 340
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true

    Overlay.modal: Rectangle {
        color: "#8C000000"
    }

    onAboutToShow: {
        var raw = voiceViewModel.accessTokens.split(",");
        var clean = [];
        for (var i = 0; i < raw.length; i++) {
            var t = raw[i].trim();
            if (t !== "") clean.push(t);
        }
        txtTokens.text = clean.join(",");
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
        spacing: 14

        Text {
            text: "Pengaturan SDC"
            font.pixelSize: 18
            font.bold: true
            color: "#111827"
        }

        Text {
            text: "Access Tokens (Pisahkan dengan koma):"
            font.pixelSize: 12
            color: "#4B5563"
        }

        TextField {
            id: txtTokens
            width: parent.width
            height: 42
            text: voiceViewModel.accessTokens
            font.pixelSize: 13
            selectByMouse: true
        }

        Item { width: 1; height: 10 }

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
                    onClicked: settingsPopup.close()
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
                width: 140
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
                        var raw = txtTokens.text.split(",");
                        var cleanList = [];
                        for (var i = 0; i < raw.length; i++) {
                            var t = raw[i].trim();
                            if (t !== "") cleanList.push(t);
                        }
                        voiceViewModel.updateAccessTokens(cleanList.join(","));
                        settingsPopup.close();
                    }
                }

                contentItem: Text {
                    text: "Simpan"
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
