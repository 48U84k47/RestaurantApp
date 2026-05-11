#include "restaurant.h"

Restaurant::Restaurant()
    : m_name("Corrindor"), m_address("123 Gourmet Avenue, Culinary City"),
    m_phone("+1 (555) 867-5309"), m_email("info@corrindor.com"),
    m_taxId("TAX-98765"), m_taxRate(8.0)
{
    initDefaultTables();
}

Restaurant::Restaurant(const QString& name, const QString& address, const QString& phone)
    : m_name(name), m_address(address), m_phone(phone),
    m_email(""), m_taxId(""), m_taxRate(8.0)
{
    initDefaultTables();
}

void Restaurant::initDefaultTables() {
    m_tables.clear();
    // Ground floor tables
    for (int i = 1; i <= 6; ++i) {
        m_tables.append(Table(i, (i <= 3) ? 2 : 4, "Ground Floor"));
    }
    // First floor tables
    for (int i = 7; i <= 12; ++i) {
        m_tables.append(Table(i, (i <= 9) ? 4 : 6, "First Floor"));
    }
    // Bar seats
    for (int i = 13; i <= 18; ++i) {
        m_tables.append(Table(i, 2, "Bar"));
    }
}

QVector<Table> Restaurant::freeTables() const {
    QVector<Table> result;
    for (const auto& t : m_tables) {
        if (!t.isOccupied) result.append(t);
    }
    return result;
}

QVector<Table> Restaurant::occupiedTables() const {
    QVector<Table> result;
    for (const auto& t : m_tables) {
        if (t.isOccupied) result.append(t);
    }
    return result;
}

Table* Restaurant::tableByNumber(int number) {
    for (auto& t : m_tables) {
        if (t.number == number) return &t;
    }
    return nullptr;
}

bool Restaurant::occupyTable(int number, int orderId) {
    Table* t = tableByNumber(number);
    if (t && !t->isOccupied) {
        t->isOccupied = true;
        t->currentOrderId = orderId;
        return true;
    }
    return false;
}

bool Restaurant::freeTable(int number) {
    Table* t = tableByNumber(number);
    if (t && t->isOccupied) {
        t->isOccupied = false;
        t->currentOrderId = -1;
        return true;
    }
    return false;
}

bool Restaurant::isTableFree(int number) const {
    for (const auto& t : m_tables) {
        if (t.number == number) return !t.isOccupied;
    }
    return false;
}

int Restaurant::freeTableCount() const {
    int count = 0;
    for (const auto& t : m_tables) {
        if (!t.isOccupied) ++count;
    }
    return count;
}
