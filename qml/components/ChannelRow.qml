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
                id: userRowRect
                width: parent.width
                height: 30
                property bool isUserSelected: voiceViewModel && voiceViewModel.selectedUserSession === modelData.session
                property bool shouldMarquee: isUserSelected || (!!modelData.isTalking)
                color: isUserSelected ? "#260284C7" : (modelData.isSelf ? "#12047857" : (userHoverArea.containsMouse ? "#0D000000" : "transparent"))
                radius: 4

                MouseArea {
                    id: userHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            userContextMenu.popup()
                        } else {
                            if (voiceViewModel) {
                                voiceViewModel.toggleSelectUser(modelData.session)
                            }
                        }
                    }
                    onDoubleClicked: {
                        userContextMenu.popup()
                    }
                }

                ToolTip.visible: userHoverArea.containsMouse
                ToolTip.delay: 400
                ToolTip.text: modelData.name + (modelData.isSelf ? " (Anda)" : "")

                Item {
                    anchors.fill: parent
                    anchors.leftMargin: (channelRowRoot.depth * 20 + 16) + 24
                    anchors.rightMargin: 16

                    // Left Side: Avatar and Name
                    Row {
                        anchors.left: parent.left
                        anchors.right: statusBadgeRow.left
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        // User Avatar (Green when idle, Blue with sound waves when talking) - Fixed
                        StatusAvatarIcon {
                            isTalking: !!modelData.isTalking
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // User Name (Marquee inside clipped viewport when selected/talking, otherwise Elide)
                        Item {
                            id: nameViewport
                            height: parent.height
                            width: Math.min(userNameText.implicitWidth, parent.width - 24)
                            clip: true
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                id: userNameText
                                text: modelData.name
                                font.pixelSize: 13
                                font.bold: modelData.isSelf || modelData.isTalking || userRowRect.isUserSelected
                                font.weight: (modelData.isSelf || userRowRect.isUserSelected) ? Font.ExtraBold : (modelData.isTalking ? Font.Bold : Font.Normal)
                                color: modelData.isTalking ? "#0284C7" : (modelData.isSelf ? "#047857" : "#1F2937")
                                anchors.verticalCenter: parent.verticalCenter
                                elide: userRowRect.shouldMarquee ? Text.ElideNone : Text.ElideRight

                                NumberAnimation on x {
                                    id: marqueeAnim
                                    running: userRowRect.shouldMarquee && userNameText.implicitWidth > nameViewport.width
                                    from: 0
                                    to: -(userNameText.implicitWidth - nameViewport.width + 10)
                                    duration: Math.max(2000, (userNameText.implicitWidth - nameViewport.width) * 35)
                                    loops: Animation.Infinite
                                    alwaysRunToEnd: false
                                    onRunningChanged: {
                                        if (!running) userNameText.x = 0
                                    }
                                }
                            }
                        }
                    }

                    // Right Side: Multiple Status Badges matching Mumble official client - Fixed
                    Row {
                        id: statusBadgeRow
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        // 1. Muted by Admin (Blue Mic Slash)
                        StatusMicIcon {
                            visible: !!modelData.isServerMuted
                            iconColor: "#0284C7"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 2. Muted by ACL (Green Mic Slash)
                        StatusMicIcon {
                            visible: !!modelData.isSuppressed
                            iconColor: "#10B981"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 3. Self Muted (Red Mic Slash)
                        StatusMicIcon {
                            visible: !!modelData.isSelfMuted
                            iconColor: "#EF4444"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 4. Deafen by Admin (Blue Speaker Slash)
                        StatusSpeakerIcon {
                            visible: !!modelData.isServerDeafened
                            iconColor: "#0284C7"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 5. Self Deafen (Red Speaker Slash)
                        StatusSpeakerIcon {
                            visible: !!modelData.isSelfDeafened
                            iconColor: "#EF4444"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 6. Registered User (Gold Checkmark)
                        Text {
                            text: "✓"
                            font.pixelSize: 13
                            font.bold: true
                            color: "#EAB308"
                            visible: !!modelData.isRegistered
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // Options 3-dots button when selected
                        Rectangle {
                            visible: userRowRect.isUserSelected && !modelData.isSelf
                            width: 20
                            height: 20
                            radius: 10
                            color: menuBtnHover.containsMouse ? "#E0F2FE" : "transparent"
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "⋮"
                                font.pixelSize: 15
                                font.bold: true
                                color: "#0284C7"
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: menuBtnHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: userContextMenu.popup()
                            }
                        }
                    }
                }

                // Context Menu for User Options (Mute / Deafen Admin)
                Menu {
                    id: userContextMenu
                    title: modelData.name

                    MenuItem {
                        text: modelData.name
                        enabled: false
                        font.bold: true
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: modelData.isServerMuted ? "Unmute Admin" : "Mute Admin"
                        visible: voiceViewModel && voiceViewModel.userRole === "super_admin"
                        onTriggered: {
                            voiceViewModel.setAdminUserMuteDeaf(modelData.session, !modelData.isServerMuted, modelData.isServerDeafened)
                        }
                    }
                    MenuItem {
                        text: modelData.isServerDeafened ? "Undeafen Admin" : "Deafen Admin"
                        visible: voiceViewModel && voiceViewModel.userRole === "super_admin"
                        onTriggered: {
                            voiceViewModel.setAdminUserMuteDeaf(modelData.session, modelData.isServerMuted, !modelData.isServerDeafened)
                        }
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

    // Inline Vector Icon Components matching Mumble official client
    component StatusMicIcon: Item {
        width: 14
        height: 14
        property color iconColor: "#DC2626"
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.strokeStyle = iconColor;
                ctx.fillStyle = iconColor;
                ctx.lineWidth = 1.3;
                ctx.lineCap = "round";

                ctx.beginPath();
                ctx.rect(5, 2, 4, 5);
                ctx.stroke();

                ctx.beginPath();
                ctx.arc(7, 6, 3.8, 0, Math.PI, false);
                ctx.stroke();

                ctx.beginPath();
                ctx.moveTo(7, 9.8);
                ctx.lineTo(7, 12);
                ctx.moveTo(4.5, 12);
                ctx.lineTo(9.5, 12);
                ctx.stroke();

                ctx.beginPath();
                ctx.moveTo(2, 2);
                ctx.lineTo(12, 12);
                ctx.stroke();
            }
        }
    }

    component StatusSpeakerIcon: Item {
        width: 14
        height: 14
        property color iconColor: "#DC2626"
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.strokeStyle = iconColor;
                ctx.fillStyle = iconColor;
                ctx.lineWidth = 1.3;
                ctx.lineCap = "round";

                ctx.beginPath();
                ctx.rect(2, 4.5, 2.5, 5);
                ctx.stroke();

                ctx.beginPath();
                ctx.moveTo(4.5, 4.5);
                ctx.lineTo(8, 2);
                ctx.lineTo(8, 12);
                ctx.lineTo(4.5, 9.5);
                ctx.closePath();
                ctx.stroke();

                ctx.beginPath();
                ctx.moveTo(2, 2);
                ctx.lineTo(12, 12);
                ctx.stroke();
            }
        }
    }

    component StatusAvatarIcon: Item {
        width: 16
        height: 16
        property bool isTalking: false
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                var col = isTalking ? "#0284C7" : "#10B981";
                ctx.strokeStyle = col;
                ctx.fillStyle = col;
                ctx.lineWidth = 1.4;
                ctx.lineCap = "round";

                // Head
                ctx.beginPath();
                ctx.arc(8, 5, 3.2, 0, 2 * Math.PI);
                ctx.fill();

                // Shoulders / body
                ctx.beginPath();
                ctx.arc(8, 15, 5.5, Math.PI * 1.15, Math.PI * 1.85);
                ctx.stroke();

                if (isTalking) {
                    ctx.beginPath();
                    ctx.arc(8, 5, 5.5, -Math.PI * 0.4, Math.PI * 0.4);
                    ctx.stroke();

                    ctx.beginPath();
                    ctx.arc(8, 5, 5.5, Math.PI * 0.6, Math.PI * 1.4);
                    ctx.stroke();
                }
            }
        }
    }
}
