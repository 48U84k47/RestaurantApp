#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <QString>
#include <QVector>
#include <QMap>

/**
 * @brief Core restaurant business entity.
 * Manages table layout, capacity, and overall restaurant state.
 */
struct Table {
    int     number;
    int     capacity;
    bool    isOccupied;
    int     currentOrderId;   // -1 if free
    QString section;          // "Indoor", "Outdoor", "Bar"

    Table() : number(0), capacity(4), isOccupied(false),
        currentOrderId(-1), section("Indoor") {}
    Table(int num, int cap, const QString& sec = "Indoor")
        : number(num), capacity(cap), isOccupied(false),
        currentOrderId(-1), section(sec) {}
};

class Restaurant {
public:
    Restaurant();
    Restaurant(const QString& name, const QString& address, const QString& phone);

    // Getters
    QString name()     const { return m_name; }
    QString address()  const { return m_address; }
    QString phone()    const { return m_phone; }
    QString email()    const { return m_email; }
    QString taxId()    const { return m_taxId; }
    double  taxRate()  const { return m_taxRate; }
    int     tableCount() const { return m_tables.size(); }

    // Table management
    QVector<Table>  allTables()     const { return m_tables; }
    QVector<Table>  freeTables()    const;
    QVector<Table>  occupiedTables() const;
    Table*          tableByNumber(int number);
    bool            occupyTable(int number, int orderId);
    bool            freeTable(int number);
    bool            isTableFree(int number) const;
    int             freeTableCount() const;
    void            initDefaultTables();

    // Settings
    void setName(const QString& n)    { m_name = n; }
    void setAddress(const QString& a) { m_address = a; }
    void setPhone(const QString& p)   { m_phone = p; }
    void setEmail(const QString& e)   { m_email = e; }
    void setTaxId(const QString& t)   { m_taxId = t; }
    void setTaxRate(double r)         { m_taxRate = r; }

private:
    QString        m_name;
    QString        m_address;
    QString        m_phone;
    QString        m_email;
    QString        m_taxId;
    double         m_taxRate;
    QVector<Table> m_tables;
};

#endif // RESTAURANT_H