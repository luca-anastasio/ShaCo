import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    signal back

    property int bufferSize: 10000

    Connections {
        target: controller
        onDataSent: {
            textArea.appendText("▶" + data)
            textAreaScroll.toEnd()
        }
        onDataReceived: {
            textArea.appendText("◀" + data)
            textAreaScroll.toEnd()
        }
        onPortClosedWithError: {
            textArea.appendText("--------\n")
            textAreaScroll.toEnd()
        }
        onPortClosed: {
            textArea.appendText("--------\n")
            textAreaScroll.toEnd()
        }
    }

    ScrollView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 3

        Flickable {
            id: textAreaScroll
            clip: true

            function toEnd() {
                contentY = -Math.min(height - contentHeight, 0)
            }

            TextArea {
                id: textArea
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                readOnly: true

                function appendText(toAppend) {
                    text += toAppend

                    if (text.length > bufferSize) {
                        text = text.substr(-bufferSize)
                    }
                }
            }
        }
    }


    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 50
        Layout.fillWidth: true

        Text {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Send command:")
        }

        TextField {
            id: textField

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 3
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            onAccepted:
                if (controller.connected && !controller.streamingGCode) {
                    controller.sendLine(textField.text)
                    textField.text = ""
                }
        }

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            enabled: controller.connected && !controller.streamingGCode

            text: qsTr("Send")

            onClicked: {
                controller.sendLine(textField.text)
                textField.text = ""
            }
        }
    }

    RowLayout {
        Layout.fillHeight: false
        Layout.preferredHeight: 50
        Layout.fillWidth: true

        Button {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.margins: 3
            text: qsTr("Back")

            onClicked: root.back()
        }

        Item {
            Layout.fillWidth: true
        }
    }
}