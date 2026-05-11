#include "menuitem.h"
#include <QStringList>

MenuItem::MenuItem()
    : m_id(-1), m_name(""), m_description(""), m_price(0.0),
    m_category(Category::All), m_available(true), m_imagePath(""), m_stock(100)
{}

MenuItem::MenuItem(int id, const QString& name, const QString& description,
                   double price, Category category, bool available,
                   const QString& imagePath)
    : m_id(id), m_name(name), m_description(description),
    m_price(price), m_category(category), m_available(available),
    m_imagePath(imagePath), m_stock(100)
{}

QString MenuItem::categoryString() const {
    return categoryToString(m_category);
}

QString MenuItem::priceString() const {
    return QString("$%1").arg(m_price, 0, 'f', 2);
}

MenuItem::Category MenuItem::categoryFromString(const QString& str) {
    if (str == "Appetizers")  return Category::Appetizers;
    if (str == "Main Course") return Category::MainCourse;
    if (str == "Desserts")    return Category::Desserts;
    if (str == "Beverages")   return Category::Beverages;
    if (str == "Specials")    return Category::Specials;
    return Category::All;
}

QString MenuItem::categoryToString(Category cat) {
    switch (cat) {
    case Category::Appetizers:  return "Appetizers";
    case Category::MainCourse:  return "Main Course";
    case Category::Desserts:    return "Desserts";
    case Category::Beverages:   return "Beverages";
    case Category::Specials:    return "Specials";
    default:                    return "All";
    }
}

QStringList MenuItem::allCategoryNames() {
    return {"All", "Appetizers", "Main Course", "Desserts", "Beverages", "Specials"};
}