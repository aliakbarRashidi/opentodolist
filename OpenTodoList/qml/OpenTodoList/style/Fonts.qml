/*
 *  OpenTodoList - A todo and task manager
 *  Copyright (C) 2014 - 2015 Martin Höher <martin@rpdev.net>
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

pragma Singleton

import QtQuick 2.2
import "."

QtObject {
    id: fonts

    property FontLoader symbols: FontLoader {
        id: symbolsFont
        source: "fontawesome-webfont.ttf"
    }

    readonly property int p: Measures.fontPointSize
    readonly property int h1: h2 * 1.1
    readonly property int h2: h3 * 1.1
    readonly property int h3: h4 * 1.1
    readonly property int h4: h5 * 1.1
    readonly property int h5: h6 * 1.1
    readonly property int h6: p * 1.1

}