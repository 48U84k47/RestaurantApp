#ifndef CORRINDOR_DATABASE_H
#define CORRINDOR_DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QString>
#include "menuitem.h"
#include "order.h"
#include "customer.h"
#include "admin.h"

/**
 * @brief Singleton database manager for SQLite persistence.
 * Handles all CRUD operations for the restaurant system.
 * Uses Qt's SQL module with prepared statements for safety.
 */
class Database : public QObject {
    Q_OBJECT

public:
    static Database& instance();

    // Lifecycle
    bool initialize(const QString& dbPath = "corrindor.db");
    bool isOpen() const;
    void close();

    // Menu items
    bool               addMenuItem(MenuItem& item);
    bool               updateMenuItem(const MenuItem& item);
    bool               deleteMenuItem(int id);
    QVector<MenuItem>  allMenuItems() const;
    QVector<MenuItem>  menuItemsByCategory(MenuItem::Category cat) const;
    MenuItem           menuItemById(int id) const;
    bool               menuItemExists(int id) const;

    // Orders
    bool            saveOrder(Order& order);
    bool            updateOrderStatus(int orderId, Order::Status status);
    QVector<Order>  allOrders() const;
    QVector<Order>  ordersByCustomer(int customerId) const;
    QVector<Order>  ordersByStatus(Order::Status status) const;
    QVector<Order>  recentOrders(int limit = 20) const;
    Order           orderById(int id) const;

    // Customers
    bool               addCustomer(Customer& customer);
    bool               updateCustomer(const Customer& customer);
    QVector<Customer>  allCustomers() const;
    Customer           customerByEmail(const QString& email) const;
    Customer           customerById(int id) const;
    bool               customerExists(const QString& email) const;

    // Admins
    bool            addAdmin(Admin& admin);
    bool            updateAdmin(const Admin& admin);
    QVector<Admin>  allAdmins() const;
    Admin           adminByUsername(const QString& username) const;
    bool            adminExists(const QString& username) const;

    // Analytics
    double  totalRevenue() const;
    double  revenueToday() const;
    int     totalOrderCount() const;
    int     orderCountToday() const;
    double  averageOrderValue() const;
    QVector<QPair<QString,double>> revenueByDay(int days = 7) const;
    QVector<QPair<QString,int>>    topSellingItems(int limit = 5) const;

    // Last error
    QString lastError() const { return m_lastError; }

signals:
    void databaseError(const QString& error);

private:
    explicit Database(QObject* parent = nullptr);
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTables();
    bool seedDefaultData();
    void loadOrderItems(Order& order) const;

    QSqlDatabase m_db;
    mutable QString m_lastError;
};

#endif // CORRINDOR_DATABASE_H
