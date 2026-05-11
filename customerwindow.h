#ifndef CUSTOMERWINDOW_H
#define CUSTOMERWINDOW_H

#include <QWidget>
#include <QVector>

#include "customer.h"
#include "menuitem.h"
#include "order.h"

class DeliveryMapWidget;
class QComboBox;
class QFrame;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTextEdit;
class QTimer;

class CustomerWindow : public QWidget {
    Q_OBJECT

public:
    explicit CustomerWindow(const Customer& customer, bool guestMode = false,
                            QWidget* parent = nullptr,
                            Order::Type initialType = Order::Type::Delivery,
                            int initialTableNumber = 0,
                            int initialPartySize = 1);

signals:
    void logoutRequested();

private slots:
    void refreshMenu();
    void addItemToCart(int menuItemId);
    void updateCart();
    void removeSelectedCartItem();
    void clearCart();
    void placeOrder();
    void onOrderTypeChanged(int index);
    void advanceDelivery();

private:
    QFrame* createMenuCard(const MenuItem& item);
    QString displayCustomerName() const;
    Order::Type selectedOrderType() const;
    void clearMenuGrid();
    void startDeliveryTracking(int orderId);
    void showOrderPlacedDialog(const Order& order) const;
    bool promptForDineInGuests();
    int allocateTableForGuests(int guests) const;
    QString tableAreaForNumber(int tableNumber) const;
    void updateTableAreaLabel();
    QString resolvedImagePath(const MenuItem& item) const;
    void playClickSound() const;
    void playSuccessSound() const;

    Customer m_customer;
    bool m_guestMode;
    bool m_dineInTableAssigned;
    Order::Type m_previousOrderType;
    QVector<MenuItem> m_menuItems;
    Order m_cart;

    QComboBox* m_categoryFilter;
    QLineEdit* m_searchEdit;
    QScrollArea* m_menuScroll;
    QWidget* m_menuContainer;
    QGridLayout* m_menuGrid;

    QListWidget* m_cartList;
    QLabel* m_subtotalLabel;
    QLabel* m_taxLabel;
    QLabel* m_deliveryFeeLabel;
    QLabel* m_totalLabel;
    QLabel* m_statusLabel;
    QLabel* m_etaLabel;
    QLabel* m_trackingStatusLabel;
    QComboBox* m_orderType;
    QSpinBox* m_tableNumber;
    QSpinBox* m_partySize;
    QLabel* m_tableAreaLabel;
    QLineEdit* m_addressEdit;
    QTextEdit* m_notesEdit;
    QPushButton* m_placeOrderButton;
    DeliveryMapWidget* m_mapWidget;
    QTimer* m_deliveryTimer;
    int m_deliveryProgress;
};

#endif // CUSTOMERWINDOW_H
