#include "toplevelitem.h"

#include <QMetaEnum>


/**
 * @brief Constructor.
 */
TopLevelItem::TopLevelItem(const QString& filename, QObject* parent) :
    ComplexItem (filename, parent),
    m_color(White),
    m_tags()
{
    connect(this, &TopLevelItem::colorChanged, this, &ComplexItem::changed);
    connect(this, &TopLevelItem::tagsChanged, this, &ComplexItem::changed);
}

/**
 * @brief Constructor.
 */
TopLevelItem::TopLevelItem(QObject* parent) : TopLevelItem(QString(), parent)
{
}

/**
 * @brief Constructor.
 */
TopLevelItem::TopLevelItem(const QDir& dir, QObject* parent) : ComplexItem(dir, parent),
    m_color(White),
    m_tags()
{
    connect(this, &TopLevelItem::colorChanged, this, &ComplexItem::changed);
    connect(this, &TopLevelItem::tagsChanged, this, &ComplexItem::changed);
}

/**
 * @brief Destructor.
 */
TopLevelItem::~TopLevelItem()
{
}

/**
 * @brief The color of the item.
 */
TopLevelItem::Color TopLevelItem::color() const
{
    return m_color;
}

/**
 * @brief Set the item color.
 */
void TopLevelItem::setColor(const Color &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
        save();
    }
}

void TopLevelItem::setColor(const QString& color)
{
    QMetaEnum e = QMetaEnum::fromType<Color>();
    QString currentColor = e.valueToKey(m_color);
    bool ok;
    Color c = static_cast<Color>(e.keysToValue(qUtf8Printable(color), &ok));
    if (ok) {
        setColor(c);
    }
}

/**
 * @brief A list of tags attached to the item.
 *
 * This is a list of tags that have been attached to the item. Tags can be used for filtering
 * items. The tags are sorted alphabetically.
 */
QStringList TopLevelItem::tags() const
{
    return m_tags;
}

void TopLevelItem::setTags(const QStringList& tags)
{
    if (m_tags != tags)
    {
        m_tags = tags;
        emit tagsChanged();
        save();
    }
}

/**
 * @brief Adds a new @p tag to the item.
 *
 * This adds a new tag to the item. If the tag already has been attached to the item before,
 * this method has no effect.
 */
void TopLevelItem::addTag(const QString& tag)
{
    if (!m_tags.contains(tag))
    {
        m_tags.append(tag);
        m_tags.sort();
        save();
        emit tagsChanged();
    }
}

/**
 * @brief Removes a tag from the item.
 *
 * This removes the tag at the given @p index from the item. The index must be
 * valid (i.e. 0 <= index < item.tags().length()). If not, this method will assert.
 */
void TopLevelItem::removeTagAt(int index)
{
    Q_ASSERT(index >= 0 && index < m_tags.length());
    m_tags.removeAt(index);
    save();
    emit tagsChanged();
}

/**
 * @brief Removes the @p tag from the item (if it is assigned).
 */
void TopLevelItem::removeTag(const QString& tag)
{
    auto index = m_tags.indexOf(tag);
    if (index >= 0) {
        removeTagAt(index);
    }
}

/**
 * @brief Returns true if the item has been tagged with the given @p tag.
 */
bool TopLevelItem::hasTag(const QString& tag) const
{
    return m_tags.contains(tag);
}

QVariantMap TopLevelItem::toMap() const
{
    auto result = ComplexItem::toMap();

    QMetaEnum e = QMetaEnum::fromType<Color>();
    result["color"] = QString(e.valueToKey(m_color));

    result["tags"] = m_tags;

    return result;
}

void TopLevelItem::fromMap(QVariantMap map)
{
    ComplexItem::fromMap(map);

    setColor(map.value("color", m_color).toString());
    setTags(map.value("tags", m_tags).toStringList());
}
