import QtQuick
import QtQuick.Controls

Column {
    id: channelRowRoot
    width: parent.width

    property var nodeData: null
    property int depth: 0
    property alias indentLevel: channelRowRoot.depth

    onNodeDataChanged: {
        if (arrowCanvas) arrowCanvas.requestPaint();
    }

    // Channel Row Item
    Rectangle {
        width: parent.width
        height: 36
        color: (nodeData && nodeData.isMyPath) ? "#10047857" : "transparent"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: channelRowRoot.depth * 20 + 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            // Expand / Collapse Arrow Icon (Canvas with perfectly matched size)
            Item {
                id: arrowContainer
                width: 20
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                visible: (nodeData && ((nodeData.children && nodeData.children.length > 0) || (nodeData.users && nodeData.users.length > 0)))

                Canvas {
                    id: arrowCanvas
                    width: 14
                    height: 14
                    anchors.centerIn: parent

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.fillStyle = (nodeData && nodeData.isMyPath) ? "#047857" : "#4B5563";
                        ctx.beginPath();
                        if (nodeData && nodeData.isExpanded) {
                            // Downward pointing triangle
                            ctx.moveTo(2, 4);
                            ctx.lineTo(12, 4);
                            ctx.lineTo(7, 10);
                        } else {
                            // Rightward pointing triangle
                            ctx.moveTo(4, 2);
                            ctx.lineTo(10, 7);
                            ctx.lineTo(4, 12);
                        }
                        ctx.closePath();
                        ctx.fill();
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (nodeData) {
                            voiceViewModel.toggleChannelExpanded(nodeData.id)
                        }
                    }
                }
            }

            // Channel Name & Total Count
            Text {
                id: channelTitle
                text: (nodeData ? nodeData.name : "") + " (" + (nodeData && nodeData.totalCount !== undefined ? nodeData.totalCount : 0) + ")"
                font.pixelSize: 14
                font.bold: true
                color: (nodeData && nodeData.isMyPath) ? "#047857" : "#111827"
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (nodeData) {
                            voiceViewModel.toggleChannelExpanded(nodeData.id)
                        }
                    }
                }
            }

            Item {
                width: 4
                height: 1
            }

            // Action: Edit / Rename Channel Icon (Hidden per user request)
            Rectangle {
                width: 0
                height: 0
                visible: false
            }

            // Action: Join Channel Icon (Green Arrow)
            Rectangle {
                width: 26
                height: 26
                radius: 4
                color: joinHoverArea.containsMouse ? "#A7F3D0" : "transparent"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: "➜"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#047857"
                    anchors.centerIn: parent
                }

                MouseArea {
                    id: joinHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (nodeData) {
                            voiceViewModel.joinChannel(nodeData.id)
                        }
                    }
                }
            }
        }
    }

    // Modal Rename Dialog
    Dialog {
        id: renameDialog
        title: "Ubah Nama Channel"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: Overlay.overlay

        TextField {
            id: txtNewName
            text: nodeData ? nodeData.name : ""
            width: 260
        }

        onAccepted: {
            if (nodeData && txtNewName.text.trimmed() !== "") {
                voiceViewModel.renameChannel(nodeData.id, txtNewName.text.trimmed())
            }
        }
    }

    // Children: Connected Users and Subchannels
    Column {
        width: parent.width
        visible: nodeData ? nodeData.isExpanded : false

        // User List under this channel
        Repeater {
            model: nodeData ? nodeData.users : []
            Rectangle {
                width: parent.width
                height: 30
                color: modelData.isSelf ? "#12047857" : "transparent"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: (channelRowRoot.depth * 20 + 16) + 24
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // User Mic State Icon
                    Text {
                        text: modelData.isMuted ? "🔇" : (modelData.isTalking ? "🎙️" : "🎙️")
                        font.pixelSize: 14
                        color: modelData.isMuted ? "#9CA3AF" : (modelData.isTalking ? "#DC2626" : "#6B7280")
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // User Deafen State Icon (if deafened)
                    Text {
                        text: "🎧"
                        font.pixelSize: 13
                        color: "#DC2626"
                        visible: modelData.isDeafened
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // User Name
                    Text {
                        text: modelData.name
                        font.pixelSize: 13
                        font.bold: modelData.isSelf || modelData.isTalking
                        font.weight: modelData.isSelf ? Font.ExtraBold : (modelData.isTalking ? Font.Bold : Font.Normal)
                        color: modelData.isTalking ? "#DC2626" : (modelData.isSelf ? "#047857" : "#1F2937")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // Subchannels Recursive Repeater
        Repeater {
            model: nodeData ? nodeData.children : []
            Loader {
                id: subLoader
                width: parent.width
                Component.onCompleted: {
                    setSource("ChannelRow.qml", {
                        "nodeData": modelData,
                        "depth": channelRowRoot.depth + 1
                    })
                }
            }
        }
    }
}
