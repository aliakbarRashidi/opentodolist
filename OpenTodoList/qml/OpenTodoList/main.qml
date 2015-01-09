/*
 *  OpenTodoList - A todo and task manager
 *  Copyright (C) 2014 - 2015  Martin Höher <martin@rpdev.net>
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

import QtQuick 2.2
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import QtQuick.Layouts 1.1

import net.rpdev.OpenTodoList.Core 1.0
import net.rpdev.OpenTodoList.DataModel 1.0

import "style" as Style
import "components" as Components
import "pages" as Pages

ApplicationWindow {
    id: root

    width: 800
    height: 600

    Component.onCompleted: {
        application.handler.requestShow.connect( function() { show(); raise(); } );
        application.handler.requestHide.connect( function() { hide(); } );
        application.handler.requestToggleWindow.connect( function() {
            if ( visible ) {
                hide();
            } else {
                show();
                raise();
            }
        } );

        width = settings.getValue( "OpenTodoList/Window/width", width );
        height = settings.getValue( "OpenTodoList/Window/height", height );
    }

    onWidthChanged: settings.setValue( "OpenTodoList/Window/width", width )
    onHeightChanged: settings.setValue( "OpenTodoList/Window/height", height )

    onActiveFocusItemChanged: defaultFocusHandler.ensureAppIsHandlingKeys()

    Settings {
        id: settings
    }

    menuBar: MenuBar {
        Menu {
            id: applicationMenu
            title: "&" + Qt.application.name
            visible: menusVisible.checked || Qt.platform.os == "osx"
            Menu {
                title: qsTr( "&Navigate" )
                MenuItem {
                    text: qsTr( "Go &Back" )
                    shortcut: StandardKey.Back
                    onTriggered: stackView.pop()
                }
                MenuItem {
                    text: qsTr( "Show &Navigation" )
                    onTriggered: navBar.toggle()
                }
            }
            Menu {
                title: qsTr( "&View" )
                MenuItem {
                    id: menusVisible
                    visible: Qt.platform.os !== "osx"
                    text: qsTr( "Show Menu Bar" )
                    checkable: true
                    checked: false
                    shortcut: qsTr( "Ctrl+M" )
                }
            }

            Menu {
                title: qsTr( "&Development Tools" )
                MenuItem {
                    text: qsTr( "Print Current Focus Item" )
                    shortcut: "Ctrl+Shift+P"
                    onTriggered: {
                        var cFI = activeFocusItem;
                        console.debug( "Current Focus item is " + cFI + "[" + cFI.objectName + "]" )
                        while ( cFI.parent !== null ) {
                            cFI = cFI.parent;
                            console.debug( "  --> " + cFI + "[" + cFI.objectName + "]" )
                        }
                    }
                }
                MenuItem {
                    text: qsTr( "Unset Current Focus Item" )
                    onTriggered: {
                        defaultFocusHandler.focus = true;
                        defaultFocusHandler.forceActiveFocus()
                        console.debug( "Current Focus Item: " +
                                      root.activeFocusItem );
                    }
                }
            }
            MenuItem {
                shortcut: StandardKey.Close
                text: qsTr( "&Close Window" )
                onTriggered: application.handler.hideWindow()
            }
            MenuItem {
                shortcut: StandardKey.Quit
                text: qsTr( "&Quit" )
                onTriggered: application.handler.terminateApplication()
            }
        }
    }

    toolBar: ToolBar {
        style: ToolBarStyle {
            background: Rectangle {
                color: Style.Colors.primary
            }
        }

        RowLayout {
            anchors.fill: parent

            Components.Symbol {
                id: toggleNavButton
                symbol: stackView.depth === 1 ? Style.Symbols.bars : Style.Symbols.singleLeft
                font.pointSize: Style.Fonts.h1
                color: Style.Colors.lightText
                onClicked: {
                    if ( stackView.depth === 1 ) {
                        navBar.toggle()
                    } else {
                        stackView.pop()
                    }
                }
            }
            Style.H1 {
                text: stackView.currentItem ? stackView.currentItem.title : ""
                Layout.fillWidth: true
                color: Style.Colors.lightText
            }
            Components.Symbol {
                symbol: Style.Symbols.verticalEllipsis
                font.pointSize: Style.Fonts.h1
                color: Style.Colors.lightText
                onClicked: applicationMenu.popup()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: todoLists

        Component {
            id: todoLists
            Pages.TodoListsPage {
                onTodoListSelected: {
                    var newTodoListPage = todos.createObject();
                    newTodoListPage.todoList = todoList;
                    stackView.push( newTodoListPage );
                }
            }
        }

        Component {
            id: todos
            Pages.TodosPage {
                onTodoSelected: {
                    var newTodoPage = todoPage.createObject();
                    newTodoPage.newTodo.fromVariant(todo.toVariant());
                    newTodoPage.todo = newTodoPage.newTodo
                    stackView.push( newTodoPage );
                }
            }
        }

        Component {
            id: todoPage
            Pages.TodoPage {
                property Todo newTodo: Todo {
                }
            }
        }
    }

    Components.NavigationBar {
        id: navBar
        anchors {
            top: parent.top
            bottom: parent.bottom
        }
        width: Math.min( Style.Measures.mWidth * 30, parent.width )
        x: -width

        function toggle() {
            navBar.state = ( navBar.state === "shown" ) ?
                        "" : "shown";
        }

        states: State {
            name: "shown"
            PropertyChanges {
                target: navBar
                x: 0
                focus: true
            }
        }

        Keys.onEscapePressed: state = ""
        Keys.onBackPressed: state = ""

        Behavior on x { SmoothedAnimation { duration: 300 } }
    }

    Item {
        id: defaultFocusHandler
        objectName: "defaultFocusHandler"
        focus: true

        Keys.onBackPressed: handleBack()
        Keys.onEscapePressed: handleBack()

        /**
          Helper function: Checks if the current focus is inside the root window and
          if not ensures that the focus is returned to the window. This is required
          to handle Back and Escape properly.
          */
        function ensureAppIsHandlingKeys() {
            var contentItem = root.contentItem;
            var currentFocusItem = root.activeFocusItem;
            var isHandling = false;
            while ( currentFocusItem !== null ) {
                if ( currentFocusItem === contentItem ) {
                    isHandling = true;
                    break;
                }
                currentFocusItem = currentFocusItem.parent;
            }
            if ( !isHandling ) {
                defaultFocusHandler.focus = true;
                defaultFocusHandler.forceActiveFocus();
            }
        }

        function handleBack() {
            if ( stackView.depth > 1 ) {
                stackView.pop();
            } else {
                switch ( Qt.platform.os ) {
                case "android":
                    application.handler.hideWindow();
                    break;
                default:
                    break;
                }
            }
        }
    }
}

/*Window {
    id: root

    width: 800
    height: 600
    color: Colors.window

    function saveViewSettings() {
        console.debug( "SaveViewSettings" );
        settings.setValue( "OpenTodoList/ViewSettings/todoSortMode", ViewSettings.todoSortMode );
        settings.setValue( "OpenTodoList/ViewSettings/showDoneTodos", ViewSettings.showDoneTodos );
    }


    onActiveFocusItemChanged: {
        if ( activeFocusItem === null ) {
            pageStack.focus = true;
        }
    }

    Component.onCompleted: {
        application.handler.requestShow.connect( function() { show(); raise(); } );
        application.handler.requestHide.connect( function() { hide(); } );
        application.handler.requestToggleWindow.connect( function() {
            if ( visible ) {
                hide();
            } else {
                show();
                raise();
            }
        } );

        width = settings.getValue( "OpenTodoList/Window/width", width );
        height = settings.getValue( "OpenTodoList/Window/height", height );
        ViewSettings.todoSortMode = settings.getValue( "OpenTodoList/ViewSettings/todoSortMode", ViewSettings.todoSortMode );
        ViewSettings.showDoneTodos = settings.getValue( "OpenTodoList/ViewSettings/showDoneTodos", ViewSettings.showDoneTodos ) === "true";

        ViewSettings.onSettingsChanged.connect( saveViewSettings );
    }

    onWidthChanged: settings.setValue( "OpenTodoList/Window/width", width )
    onHeightChanged: settings.setValue( "OpenTodoList/Window/height", height )

    Settings {
        id: settings
    }

    PageStack {
        id: pageStack

        anchors.fill: parent
        focus: true

        onLastPageClosing: {
            if ( Qt.platform.os === "android" ) {
                // TODO: Stop main activity here
                application.handler.hideWindow()
            }
        }

        TodoListsPage {
            onTodoSelected: {
                var component = Qt.createComponent( "TodoPage.qml" );
                if ( component.status === Component.Ready ) {
                    var page = component.createObject( pageStack );
                    page.todo.fromVariant( todo.toVariant() );
                } else {
                    console.error( component.errorString() );
                }
            }
        }
    }

    Shortcut {
        keySequence: fromStandardKey( StandardKey.Quit )
        onTriggered: {
            application.handler.terminateApplication()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Style.Style.colors.
    }
}*/

