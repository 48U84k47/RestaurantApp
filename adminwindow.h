#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "admin.h"
#include "menuitem.h"
#include "order.h"
#include "database.h"

class QResizeEvent;

/**
 * @brief Full-featured admin panel for restaurant management.
 * Features dashboard analytics, menu management, order tracking,
 * customer management, and table monitoring.
 */
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(const Admin& admin, QWidget* parent = nullptr);
    ~AdminWindow() override = default;

signals:
    void logoutRequested();

private slots:
    void navigateTo(int page);
    void refreshDashboard();
    void refreshMenuTable();
    void refreshOrdersTable();

    // Menu CRUD
    void onAddMenuItem();
    void onAddDeal();
    void onEditMenuItem();
    void onDeleteMenuItem();
    void onMenuSearch(const QString& query);
    void onCategoryFilter(int idx);

    // Order management
    void onUpdateOrderStatus();
    void onViewOrderDetails();

    // General
    void onLogout();
    void showNotification(const QString& msg, bool success = true);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void setupUi();
    void setupSidebar();
    void setupDashboardPage();
    void setupMenuPage();
    void setupOrdersPage();
    void setupTablesPage();
    void setupCustomersPage();
    void setupConnections();

    // Dashboard helpers
    QFrame* createStatCard(const QString& title, const QString& value,
                           const QString& icon, const QString& color);
    void updateStatCard(QFrame* card, const QString& value);
    void refreshTableGrid();

    // Menu dialog
    bool showMenuItemDialog(MenuItem& item, bool isEdit = false);

    // UI components
    Admin           m_admin;

    // Layout
    QWidget*        m_centralWidget;
    QHBoxLayout*    m_mainLayout;
    QFrame*         m_sidebar;
    QStackedWidget* m_pages;

    // Sidebar buttons
    QPushButton*    m_btnDashboard;
    QPushButton*    m_btnMenu;
    QPushButton*    m_btnOrders;
    QPushButton*    m_btnTables;
    QPushButton*    m_btnCustomers;
    QPushButton*    m_btnLogout;
    QLabel*         m_adminNameLabel;
    QLabel*         m_adminRoleLabel;

    // Dashboard
    QFrame*         m_cardRevenue;
    QFrame*         m_cardOrders;
    QFrame*         m_cardToday;
    QFrame*         m_cardAvg;
    QTableWidget*   m_recentOrdersTable;
    QWidget*        m_topItemsWidget;

    // Menu page
    QTableWidget*   m_menuTable;
    QLineEdit*      m_menuSearch;
    QComboBox*      m_menuCategoryFilter;
    QPushButton*    m_addItemBtn;
    QPushButton*    m_addDealBtn;
    QPushButton*    m_editItemBtn;
    QPushButton*    m_deleteItemBtn;

    // Orders page
    QTableWidget*   m_ordersTable;
    QComboBox*      m_orderStatusFilter;
    QPushButton*    m_updateStatusBtn;
    QPushButton*    m_viewOrderBtn;

    // Tables page
    QWidget*        m_tablesGrid;

    // Notification
    QLabel*         m_notification;
    QTimer*         m_notifTimer;

    int             m_currentPage;
    QVector<MenuItem> m_menuItems;
    QVector<Order>    m_orders;
};

#endif // ADMINWINDOW_H
