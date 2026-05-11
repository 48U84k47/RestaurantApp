#include "order.h"

Order::Order()
    : m_id(-1), m_customerId(-1), m_customerName("Guest"),
    m_type(Type::DineIn), m_status(Status::Pending),
    m_timestamp(QDateTime::currentDateTime()),
    m_tableNumber(0), m_deliveryAddress(""),
    m_discount(0.0), m_taxRate(8.0), m_notes("")
{}

Order::Order(int id, int customerId, Type type)
    : m_id(id), m_customerId(customerId), m_customerName("Guest"),
    m_type(type), m_status(Status::Pending),
    m_timestamp(QDateTime::currentDateTime()),
    m_tableNumber(0), m_deliveryAddress(""),
    m_discount(0.0), m_taxRate(8.0), m_notes("")
{}

void Order::addItem(const MenuItem& item, int qty) {
    // Use index loop — avoids QVector copy-on-write detach issues
    // that can silently occur with range-for auto& on a shared vector.
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].item.id() == item.id()) {
            m_items[i].quantity += qty;
            return;
        }
    }
    m_items.append({item, qty});
}

void Order::removeItem(int menuItemId) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].item.id() == menuItemId) {
            m_items.removeAt(i);
            return;
        }
    }
}

void Order::updateQuantity(int menuItemId, int qty) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].item.id() == menuItemId) {
            if (qty <= 0) {
                m_items.removeAt(i);
            } else {
                m_items[i].quantity = qty;
            }
            return;
        }
    }
}

void Order::clearItems() {
    m_items.clear();
}

int Order::itemCount() const {
    int total = 0;
    for (const auto& oi : m_items) total += oi.quantity;
    return total;
}

double Order::subtotal() const {
    double total = 0.0;
    for (const auto& oi : m_items) total += oi.subtotal();
    return total;
}

double Order::taxAmount() const {
    return subtotal() * (m_taxRate / 100.0);
}

double Order::discountAmt() const {
    return subtotal() * (m_discount / 100.0);
}

double Order::grandTotal() const {
    return subtotal() + taxAmount() - discountAmt();
}

QString Order::typeString()   const { return typeToString(m_type); }
QString Order::statusString() const { return statusToString(m_status); }
QString Order::totalString()  const { return QString("$%1").arg(grandTotal(), 0, 'f', 2); }

QString Order::typeToString(Type t) {
    switch (t) {
    case Type::DineIn:   return "Dine-In";
    case Type::Takeaway: return "Takeaway";
    case Type::Delivery: return "Delivery";
    default:             return "Unknown";
    }
}

QString Order::statusToString(Status s) {
    switch (s) {
    case Status::Pending:   return "Pending";
    case Status::Preparing: return "Preparing";
    case Status::Ready:     return "Ready";
    case Status::Delivered: return "Delivered";
    case Status::Cancelled: return "Cancelled";
    default:                return "Unknown";
    }
}

Order::Type Order::typeFromString(const QString& s) {
    if (s == "Dine-In")   return Type::DineIn;
    if (s == "Takeaway")  return Type::Takeaway;
    if (s == "Delivery")  return Type::Delivery;
    return Type::DineIn;
}

Order::Status Order::statusFromString(const QString& s) {
    if (s == "Pending")   return Status::Pending;
    if (s == "Preparing") return Status::Preparing;
    if (s == "Ready")     return Status::Ready;
    if (s == "Delivered") return Status::Delivered;
    if (s == "Cancelled") return Status::Cancelled;
    return Status::Pending;
}