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

Item {
    id: label

    property alias label: text.text
    property alias font: text.font
    property color color: "black"
    property color activeColor: colors.primaryLighter2
    property bool active: false

    signal clicked

    width: 100
    height: text.height

    Text {
        id: text

        font.pointSize: fonts.h3
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: label.width
        color: label.active ? label.activeColor : label.color
    }

    MouseArea {
        id: mouseArea

        anchors.fill: label
        cursorShape: Qt.PointingHandCursor

        onClicked: label.clicked()
    }
}
