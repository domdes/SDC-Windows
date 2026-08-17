import QtQuick
import QtQuick.Controls

Rectangle {
    id: pttRoot
    width: parent.width
    height: 52
    radius: 26
    color: voiceViewModel.isPttActive ? "#EF4444" : "#047857"

    Text {
        text: voiceViewModel.isPttActive ? "🔴 SEDANG BICARA (PTT AKTIF)" : "TEKAN UNTUK BICARA"
        font.pixelSize: 15
        font.bold: true
        color: "#FFFFFF"
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onPressed: voiceViewModel.setPttActive(true)
        onReleased: voiceViewModel.setPttActive(false)
        onCanceled: voiceViewModel.setPttActive(false)
    }
}
