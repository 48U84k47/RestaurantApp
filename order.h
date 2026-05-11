#ifndef BELLAVISTA_ORDER_H
#define BELLAVISTA_ORDER_H

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QVector>
#include <QPair>
#include "menuitem.h"

/**
 * @brief Represents a restaurant order with line items, status and billing.
 * Supports Dine-In, Takeaway and Delivery order types.
 */
class Order {
public:
    enum class Type {
        DineIn,
        Takeaway,
        Delivery
    };

    enum class Status {
        Pending,
        Preparing,
        Ready,
        Delivered,
        Cancelled
    };

    // Each cart line: item + quantity
    struct OrderItem {
        MenuItem item;
        int      quantity;
        double   subtotal() const { return item.price() * quantity; }
    };

    Order();
    Order(int id, int customerId, Type type);

    // Getters
    int             id()           const { return m_id; }
    int             customerId()   const { return m_customerId; }
    Type            type()         const { return m_type; }
    Status          status()       const { return m_status; }
    QDateTime       timestamp()    const { return m_timestamp; }
    QVector<OrderItem> items()     const { return m_items; }
    int             tableNumber()  const { return m_tableNumber; }
    QString         deliveryAddr() const { return m_deliveryAddress; }
    double          discount()     const { return m_discount; }
    double          taxRate()      const { return m_taxRate; }
    QString         notes()        const { return m_notes; }
    QString         customerName() const { return m_customerName; }

    // Setters
    void setId(int id)                          { m_id = id; }
    void setCustomerId(int cid)                 { m_customerId = cid; }
    void setType(Type t)                        { m_type = t; }
    void setStatus(Status s)                    { m_status = s; }
    void setTableNumber(int tn)                 { m_tableNumber = tn; }
    void setDeliveryAddress(const QString& a)   { m_deliveryAddress = a; }
    void setDiscount(double d)                  { m_discount = d; }
    void setTaxRate(double r)                   { m_taxRate = r; }
    void setNotes(const QString& n)             { m_notes = n; }
    void setCustomerName(const QString& name)   { m_customerName = name; }
    void setTimestamp(const QDateTime& dt)      { m_timestamp = dt; }

    // Cart operations
    void addItem(const MenuItem& item, int qty = 1);
    void removeItem(int menuItemId);
    void updateQuantity(int menuItemId, int qty);
    void clearItems();
    bool isEmpty() const { return m_items.isEmpty(); }
    int  itemCount() const;

    // Financial calculations
    double subtotal()   const;
    double taxAmount()  const;
    double discountAmt() const;
    double grandTotal() const;

    // Utility strings
    QString typeString()   const;
    QString statusString() const;
    QString totalString()  const;

    static QString typeToString(Type t);
    static QString statusToString(Status s);
    static Type    typeFromString(const QString& s);
    static Status  statusFromString(const QString& s);

private:
    int               m_id;
    int               m_customerId;
    QString           m_customerName;
    Type              m_type;
    Status            m_status;
    QDateTime         m_timestamp;
    QVector<OrderItem> m_items;
    int               m_tableNumber;
    QString           m_deliveryAddress;
    double            m_discount;     // percentage 0-100
    double            m_taxRate;      // percentage, default 8.0
    QString           m_notes;
};

#endif // BELLAVISTA_ORDER_H