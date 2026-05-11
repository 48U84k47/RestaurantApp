#include "mainwindow.h"

#include "adminwindow.h"
#include "customerwindow.h"
#include "database.h"
#include "loginwindow.h"
#include "restaurant.h"

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSpinBox>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_loginWindow(nullptr),
      m_adminWindow(nullptr),
      m_customerWindow(nullptr),
      m_navigationBar(nullptr),
      m_backButton(nullptr),
      m_forwardButton(nullptr),
      m_historyIndex(-1),
      m_restoringHistory(false)
{
    setWindowTitle("Corrindor");
    setMinimumSize(980, 680);
    setupNavigationBar();
    showLogin();
}

void MainWindow::showLogin() {
    m_adminWindow = nullptr;
    m_customerWindow = nullptr;

    m_loginWindow = new LoginWindow(this);
    setScrollableCentral(m_loginWindow);
    resize(980, 680);
    pushHistory({ScreenKind::Welcome, Order::Type::Delivery, Admin(), Customer(), true});

    connect(m_loginWindow, &LoginWindow::adminLoggedIn,
            this, &MainWindow::showAdminWindow);
    connect(m_loginWindow, &LoginWindow::customerLoggedIn,
            this, &MainWindow::showCustomerWindow);
    connect(m_loginWindow, &LoginWindow::guestMode,
            this, [this] { showGuestWindow(); });
    connect(m_loginWindow, &LoginWindow::orderModeSelected,
            this, [this](Order::Type type) { showGuestWindow(type); });
}

void MainWindow::showAdminWindow(const Admin& admin) {
    m_loginWindow = nullptr;
    m_customerWindow = nullptr;

    m_adminWindow = new AdminWindow(admin, this);
    m_adminWindow->setMinimumSize(1200, 780);
    setScrollableCentral(m_adminWindow);
    resize(m_adminWindow->size());
    pushHistory({ScreenKind::Admin, Order::Type::Delivery, admin, Customer(), false});

    connect(m_adminWindow, &AdminWindow::logoutRequested,
            this, &MainWindow::showLogin);
}

void MainWindow::showCustomerWindow(const Customer& customer) {
    m_loginWindow = nullptr;
    m_adminWindow = nullptr;

    m_customerWindow = new CustomerWindow(customer, false, this, Order::Type::Delivery);
    m_customerWindow->setMinimumSize(1180, 860);
    setScrollableCentral(m_customerWindow);
    resize(1120, 760);
    pushHistory({ScreenKind::Customer, Order::Type::Delivery, Admin(), customer, false});

    connect(m_customerWindow, &CustomerWindow::logoutRequested,
            this, &MainWindow::showLogin);
}

void MainWindow::showGuestWindow() {
    showGuestWindow(Order::Type::Delivery);
}

void MainWindow::showGuestWindow(Order::Type type) {
    int tableNumber = 0;
    int partySize = 1;
    if (type == Order::Type::DineIn && !confirmDineInTable(tableNumber, partySize))
        return;

    m_loginWindow = nullptr;
    m_adminWindow = nullptr;

    m_customerWindow = new CustomerWindow(Customer(), true, this, type, tableNumber, partySize);
    m_customerWindow->setMinimumSize(1180, 860);
    setScrollableCentral(m_customerWindow);
    resize(1120, 760);
    pushHistory({ScreenKind::Customer, type, Admin(), Customer(), true});

    connect(m_customerWindow, &CustomerWindow::logoutRequested,
            this, &MainWindow::showLogin);
}

bool MainWindow::confirmDineInTable(int& tableNumber, int& partySize) {
    QDialog dialog(this);
    dialog.setWindowTitle("Confirm table");
    dialog.setModal(true);
    dialog.setFixedSize(430, 270);
    dialog.setObjectName("orderDialog");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 24);
    layout->setSpacing(14);

    auto* title = new QLabel("Confirm your table", &dialog);
    title->setObjectName("dialogTitle");
    title->setAlignment(Qt::AlignCenter);

    auto* detail = new QLabel("Choose guest count first. We will assign a table and area before opening dine-in ordering.", &dialog);
    detail->setObjectName("dialogDetails");
    detail->setWordWrap(true);
    detail->setAlignment(Qt::AlignCenter);

    auto* guests = new QSpinBox(&dialog);
    guests->setRange(1, 18);
    guests->setValue(1);
    guests->setPrefix("Guests ");
    guests->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    guests->setMinimumHeight(42);

    auto* assigned = new QLabel(&dialog);
    assigned->setObjectName("dialogBadge");
    assigned->setAlignment(Qt::AlignCenter);

    auto updateAssigned = [&]() {
        const int table = allocateTableForGuests(guests->value());
        assigned->setText(table > 0
                              ? QString("Table %1 - %2").arg(table).arg(tableAreaForNumber(table))
                              : "No table is available for this party size");
    };
    updateAssigned();

    auto* buttons = new QHBoxLayout();
    auto* cancel = new QPushButton("Cancel", &dialog);
    cancel->setObjectName("ghostBtn");
    auto* confirm = new QPushButton("Confirm table", &dialog);
    confirm->setObjectName("primaryBtn");
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);

    layout->addWidget(title);
    layout->addWidget(detail);
    layout->addWidget(guests);
    layout->addWidget(assigned);
    layout->addStretch();
    layout->addLayout(buttons);

    connect(guests, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, updateAssigned);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(confirm, &QPushButton::clicked, [&]() {
        const int table = allocateTableForGuests(guests->value());
        if (table <= 0)
            return;
        tableNumber = table;
        partySize = guests->value();
        dialog.accept();
    });

    return dialog.exec() == QDialog::Accepted;
}

int MainWindow::allocateTableForGuests(int guests) const {
    QSet<int> occupied;
    for (const auto& order : Database::instance().allOrders()) {
        if (order.type() == Order::Type::DineIn
            && order.status() != Order::Status::Delivered
            && order.status() != Order::Status::Cancelled
            && order.tableNumber() > 0) {
            occupied.insert(order.tableNumber());
        }
    }

    Restaurant rest;
    int bestTable = 0;
    int bestCapacity = 999;
    for (const auto& table : rest.allTables()) {
        if (occupied.contains(table.number) || table.capacity < guests)
            continue;
        if (table.capacity < bestCapacity) {
            bestTable = table.number;
            bestCapacity = table.capacity;
        }
    }
    return bestTable;
}

QString MainWindow::tableAreaForNumber(int tableNumber) const {
    Restaurant rest;
    for (const auto& table : rest.allTables()) {
        if (table.number == tableNumber)
            return table.section;
    }
    return "Dining Area";
}

void MainWindow::setScrollableCentral(QWidget* widget) {
    auto* scroll = new QScrollArea(this);
    scroll->setWidget(widget);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    setCentralWidget(scroll);
}

void MainWindow::setupNavigationBar() {
    m_navigationBar = addToolBar("Navigation");
    m_navigationBar->setMovable(false);
    m_navigationBar->setFloatable(false);
    m_navigationBar->setObjectName("navigationBar");

    m_backButton = new QPushButton(QString::fromUtf8("←"), this);
    m_backButton->setObjectName("navBtn");
    m_backButton->setToolTip("Back");
    m_forwardButton = new QPushButton(QString::fromUtf8("→"), this);
    m_forwardButton->setObjectName("navBtn");
    m_forwardButton->setToolTip("Forward");

    m_navigationBar->addWidget(m_backButton);
    m_navigationBar->addWidget(m_forwardButton);

    connect(m_backButton, &QPushButton::clicked, this, &MainWindow::goBack);
    connect(m_forwardButton, &QPushButton::clicked, this, &MainWindow::goForward);
    updateNavigationButtons();
}

void MainWindow::pushHistory(const NavigationEntry& entry) {
    if (m_restoringHistory) {
        updateNavigationButtons();
        return;
    }

    while (m_history.size() > m_historyIndex + 1)
        m_history.removeLast();

    m_history.append(entry);
    m_historyIndex = m_history.size() - 1;
    updateNavigationButtons();
}

void MainWindow::goBack() {
    if (m_historyIndex <= 0)
        return;
    m_historyIndex--;
    restoreHistory(m_history[m_historyIndex]);
}

void MainWindow::goForward() {
    if (m_historyIndex >= m_history.size() - 1)
        return;
    m_historyIndex++;
    restoreHistory(m_history[m_historyIndex]);
}

void MainWindow::restoreHistory(const NavigationEntry& entry) {
    m_restoringHistory = true;
    switch (entry.kind) {
    case ScreenKind::Welcome:
        showLogin();
        break;
    case ScreenKind::Admin:
        showAdminWindow(entry.admin);
        break;
    case ScreenKind::Customer:
        if (entry.guest)
            showGuestWindow(entry.orderType);
        else
            showCustomerWindow(entry.customer);
        break;
    }
    m_restoringHistory = false;
    updateNavigationButtons();
}

void MainWindow::updateNavigationButtons() {
    if (!m_backButton || !m_forwardButton)
        return;
    const bool canGoBack = m_historyIndex > 0;
    const bool canGoForward = m_historyIndex >= 0 && m_historyIndex < m_history.size() - 1;
    m_backButton->setVisible(canGoBack);
    m_backButton->setEnabled(canGoBack);
    m_forwardButton->setVisible(canGoForward);
    m_forwardButton->setEnabled(canGoForward);
    if (m_navigationBar)
        m_navigationBar->setVisible(canGoBack || canGoForward);
}
