/*
 *  OpenTodoListDesktopQml - Desktop QML frontend for OpenTodoList
 *  Copyright (C) 2013  Martin Höher <martin@rpdev.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import "../js/Utils.js" as Utils

Item {
    id: indicator

    property int priority: 0
    property int sizeOnFocus: layout.minimumButtonHeight * 4

    signal selectedPriority(int priority)

    Keys.onEscapePressed: focus = false

    width: text.width + text.height
    height: width

    Item {
        id: helper
        readonly property color noColor: "#00000000"
        property color hoveredColor: noColor
        function noColorSelected() { return hoveredColor === noColor; }
        Behavior on hoveredColor { ColorAnimation { duration: 50 } }
    }

    Image {
        property color color: Utils.PriorityColors[priority]

        anchors {
            fill: parent
        }
        sourceSize.width: indicator.sizeOnFocus
        sourceSize.height: indicator.sizeOnFocus
        source: "image://primitives/pie/percentage=100,fill=" + color
    }

    MouseArea {
        id: indicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: indicator.focus = !indicator.focus
    }

    Text {
        id: text
        opacity: 1.0 - inner.opacity
        anchors.centerIn: parent
        text: indicator.priority >= 0 ? "Priority " + indicator.priority : "No Priority"
        font.pointSize: fonts.p
        color: colors.fontColorFor( Utils.PriorityColors[indicator.priority] )
    }

    Item {
        id: inner

        opacity: 0
        anchors {
            fill: parent
            margins: 5
        }

        Repeater {
            model: 12
            Item {
                opacity: inner.opacity
                visible: opacity !== 0
                width: indicator.width / 6
                height: indicator.height * 9 / 10
                rotation: index * 360 / 12
                anchors.horizontalCenter: parent.horizontalCenter
                Image {
                    width: parent.width
                    height: parent.width
                    sourceSize.width: layout.minimumButtonHeight
                    sourceSize.height: layout.minimumButtonHeight
                    source: "image://primitives/pie/percentage=100,fill=" +
                            text.color
                }
                Image {
                    width: parent.width - 4
                    height: parent.width -4
                    x: 2
                    y: 2
                    sourceSize.width: layout.minimumButtonHeight
                    sourceSize.height: layout.minimumButtonHeight
                    source: "image://primitives/pie/percentage=100,fill=" +
                            Utils.PriorityColors[index-1]
                }
                MouseArea {
                    width: parent.width
                    height: parent.width
                    hoverEnabled: true
                    onClicked: {
                        indicator.selectedPriority( index - 1 )
                        indicator.focus = false;
                    }
                    onContainsMouseChanged: helper.hoveredColor =
                                            containsMouse ?
                                                Utils.PriorityColors[index-1] :
                                                helper.noColor
                }
            }
        }
    }

    states: [
        State {
            name: "focused"
            when: indicator.focus
            PropertyChanges {
                target: indicator
                width: indicator.sizeOnFocus
            }
            PropertyChanges {
                target: inner
                opacity: 1.0
            }
        }
    ]


    transitions: [
        Transition {
            from: ""
            to: "focused"
            reversible: true
            SequentialAnimation {
                NumberAnimation {
                    target: indicator
                    property: "width"
                    duration: 200
                }
                NumberAnimation {
                    target: inner
                    property: "opacity"
                    duration: 200
                }
            }
        }
    ]
}
