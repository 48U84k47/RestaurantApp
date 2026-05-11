#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "admin.h"
#include "customer.h"
#include "order.h"

class AdminWindow;
class CustomerWindow;
class LoginWindow;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void showLogin();
    void showAdminWindow(const Admin& admin);
    void showCustomerWindow(const Customer& customer);
    void showGuestWindow();
    void showGuestWindow(Order::Type type);
    void goBack();
    void goForward();

private:
    enum class ScreenKind {
        Welcome,
        Admin,
        Customer
    };

    struct NavigationEntry {
        ScreenKind kind;
        Order::Type orderType;
        Admin admin;
        Customer customer;
        bool guest;
    };

    void setupNavigationBar();
    void setScrollableCentral(QWidget* widget);
    bool confirmDineInTable(int& tableNumber, int& partySize);
    int allocateTableForGuests(int guests) const;
    QString tableAreaForNumber(int tableNumber) const;
    void pushHistory(const NavigationEntry& entry);
    void restoreHistory(const NavigationEntry& entry);
    void updateNavigationButtons();

    LoginWindow* m_loginWindow;
    AdminWindow* m_adminWindow;
    CustomerWindow* m_customerWindow;
    QPushButton* m_backButton;
    QPushButton* m_forwardButton;
    QVector<NavigationEntry> m_history;
    int m_historyIndex;
    bool m_restoringHistory;
};

#endif // MAINWINDOW_H
