#ifndef TODOLIST_H
#define TODOLIST_H

#include "toplevelitem.h"

#include <QObject>
#include <QPointer>


class Library;
class Todo;

class TodoList : public TopLevelItem
{
    Q_OBJECT

    friend class Library;

public:

    explicit TodoList(QObject* parent = nullptr);
    explicit TodoList(const QString &filename, QObject *parent = nullptr);
    explicit TodoList(const QDir &dir, QObject *parent = nullptr);
    virtual ~TodoList();

    Q_INVOKABLE Todo* addTodo();

signals:

public slots:

protected:

private:

    Library* m_library;

private slots:

};

typedef QSharedPointer<TodoList> TodoListPtr;

#endif // TODOLIST_H
