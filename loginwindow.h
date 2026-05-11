#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include "admin.h"
#include "customer.h"
#include "order.h"

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override = default;

signals:
    void adminLoggedIn(const Admin& admin);
    void customerLoggedIn(const Customer& customer);
    void guestMode();
    void orderModeSelected(Order::Type type);

private slots:
    void onAdminLogin();
    void showAdminLoginDialog();
    void showError(const QString& msg);
    void clearError();
    void animateLogo();

private:
    void setupUi();
    void setupConnections();
    bool validateAdminLogin(const QString& username, const QString& password, Admin& admin, QString& error) const;

    QLabel*      m_logoLabel;
    QLabel*      m_errorLabel;
    QPushButton* m_dineInBtn;
    QPushButton* m_deliveryBtn;
    QPushButton* m_takeawayBtn;
    QPushButton* m_adminBtn;
    QTimer*      m_errorTimer;
    QTimer*      m_logoTimer;
    int          m_logoAnim;
};

#endif // LOGINWINDOW_H
