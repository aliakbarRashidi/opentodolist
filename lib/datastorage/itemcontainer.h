#ifndef ITEMCONTAINER_H
#define ITEMCONTAINER_H

#include <QObject>
#include <QList>
#include <QHash>

#include "item.h"

/**
 * @brief A container for items.
 *
 * The ItemContainer class is used to store a set of items. Items are handled as
 * shared pointers to the Item class. The container has methods to add and remove items as
 * well as accessing them by their index:
 *
 * @code
 * ItemContainer c;
 * auto item = loadItem();
 * c.addItem(item);
 * auto count = c.count; // should be 1
 * for (auto i = 0; i < count; ++i) {
 *     auto match = (item == c.item(i));
 * }
 * c.deleteItem(item); // Remove the item from the collection
 * @endcode
 *
 * In addition, the container provides an update method, which can be used to update the
 * properties of one of the contained items with another item, matching them via their
 * uid:
 *
 * @code
 * ItemContainer c;
 * auto anItem = loadItem();
 * c.addItem(anItem);
 * auto anotherItem = loadItem();
 * c.updateItem(anotherItem);
 * @endcode
 */
class ItemContainer : public QObject
{
    Q_OBJECT
public:
    explicit ItemContainer(QObject *parent = 0);
    virtual ~ItemContainer();

    int count() const;
    ItemPtr item(int index) const;
    void addItem(ItemPtr item);
    void updateItem(ItemPtr item);
    void deleteItem(ItemPtr item);

signals:

    /**
     * @brief An item has been added at the given @p index.
     */
    void itemAdded(int index);

    /**
     * @brief An item has been removed at the given @p index.
     */
    void itemDeleted(int index);

public slots:

private:

    QList<ItemPtr> m_items;
    QHash<QUuid, ItemPtr> m_uidMap;

};

#endif // ITEMCONTAINER_H
