#ifndef MENUITEM_H
#define MENUITEM_H

#include <QString>
#include <QPixmap>

/**
 * @brief Represents a single item on the restaurant menu.
 * Encapsulates all data for a food/drink item including pricing,
 * category, availability and optional image.
 */
class MenuItem {
public:
    // Categories for menu classification
    enum class Category {
        Appetizers,
        MainCourse,
        Desserts,
        Beverages,
        Specials,
        All
    };

    // Constructors
    MenuItem();
    MenuItem(int id, const QString& name, const QString& description,
             double price, Category category, bool available = true,
             const QString& imagePath = "");

    // Getters
    int         id()          const { return m_id; }
    QString     name()        const { return m_name; }
    QString     description() const { return m_description; }
    double      price()       const { return m_price; }
    Category    category()    const { return m_category; }
    bool        isAvailable() const { return m_available; }
    QString     imagePath()   const { return m_imagePath; }
    int         stock()       const { return m_stock; }

    // Setters
    void setId(int id)                        { m_id = id; }
    void setName(const QString& name)         { m_name = name; }
    void setDescription(const QString& desc)  { m_description = desc; }
    void setPrice(double price)               { m_price = price; }
    void setCategory(Category cat)            { m_category = cat; }
    void setAvailable(bool available)         { m_available = available; }
    void setImagePath(const QString& path)    { m_imagePath = path; }
    void setStock(int stock)                  { m_stock = stock; }

    // Utility
    QString categoryString() const;
    QString priceString()    const;

    static Category categoryFromString(const QString& str);
    static QString  categoryToString(Category cat);
    static QStringList allCategoryNames();

private:
    int       m_id;
    QString   m_name;
    QString   m_description;
    double    m_price;
    Category  m_category;
    bool      m_available;
    QString   m_imagePath;
    int       m_stock;
};

#endif // MENUITEM_H