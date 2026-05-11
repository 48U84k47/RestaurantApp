#include "adminwindow.h"
#include "restaurant.h"
#include <QAbstractSpinBox>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QScrollArea>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QScreen>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QSet>

AdminWindow::AdminWindow(const Admin& admin, QWidget* parent)
    : QMainWindow(parent), m_admin(admin), m_currentPage(0)
{
    setWindowTitle("Corrindor — Admin Panel");
    setMinimumSize(1200, 720);
    resize(1400, 800);

    QRect geo = QApplication::primaryScreen()->geometry();
    move((geo.width() - width()) / 2, (geo.height() - height()) / 2);

    setupUi();
    setupConnections();
    navigateTo(0);
}

void AdminWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupSidebar();

    m_pages = new QStackedWidget(m_centralWidget);
    setupDashboardPage();
    setupMenuPage();
    setupOrdersPage();
    setupTablesPage();
    setupCustomersPage();

    m_mainLayout->addWidget(m_sidebar);
    m_mainLayout->addWidget(m_pages);

    // Floating notification
    m_notification = new QLabel(this);
    m_notification->setStyleSheet(
        "background-color: #166534; color: #4ade80; border-radius: 10px;"
        "padding: 10px 20px; font-size: 13px; font-weight: 600;");
    m_notification->setAlignment(Qt::AlignCenter);
    m_notification->hide();

    m_notifTimer = new QTimer(this);
    m_notifTimer->setSingleShot(true);
    connect(m_notifTimer, &QTimer::timeout, m_notification, &QLabel::hide);
}

void AdminWindow::setupSidebar() {
    m_sidebar = new QFrame(m_centralWidget);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(240);

    auto* layout = new QVBoxLayout(m_sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Branding area
    auto* brand = new QFrame(m_sidebar);
    brand->setStyleSheet("background-color: #fff7ed; border-bottom: 1px solid #fed7aa;");
    brand->setFixedHeight(100);
    auto* brandL = new QVBoxLayout(brand);
    brandL->setAlignment(Qt::AlignCenter);

    auto* logoLbl = new QLabel("🍽 CORRINDOR", brand);
    logoLbl->setStyleSheet("font-size: 16px; font-weight: 800; color: #f97316; letter-spacing: 2px;");
    logoLbl->setAlignment(Qt::AlignCenter);
    auto* typeLbl = new QLabel("Management System", brand);
    typeLbl->setStyleSheet("font-size: 10px; color: #6b7280; letter-spacing: 1px;");
    typeLbl->setAlignment(Qt::AlignCenter);
    brandL->addWidget(logoLbl);
    brandL->addWidget(typeLbl);
    layout->addWidget(brand);

    // Admin info
    auto* adminInfo = new QFrame(m_sidebar);
    adminInfo->setStyleSheet("background-color: #ffffff; padding: 12px; border-bottom: 1px solid #fed7aa;");
    auto* adminL = new QHBoxLayout(adminInfo);
    adminL->setContentsMargins(16, 12, 16, 12);
    auto* avatar = new QLabel("👤", adminInfo);
    avatar->setStyleSheet("font-size: 28px;");
    auto* infoV = new QVBoxLayout();
    m_adminNameLabel = new QLabel(m_admin.displayName(), adminInfo);
    m_adminNameLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #111827;");
    m_adminRoleLabel = new QLabel(m_admin.roleString(), adminInfo);
    m_adminRoleLabel->setStyleSheet("font-size: 11px; color: #f97316; font-weight: 800;");
    infoV->addWidget(m_adminNameLabel);
    infoV->addWidget(m_adminRoleLabel);
    adminL->addWidget(avatar);
    adminL->addLayout(infoV);
    layout->addWidget(adminInfo);

    // Nav label
    auto* navLbl = new QLabel("NAVIGATION", m_sidebar);
    navLbl->setStyleSheet("color: #3d4560; font-size: 10px; font-weight: 700; "
                          "letter-spacing: 1.5px; padding: 16px 16px 8px;");
    layout->addWidget(navLbl);

    // Nav buttons
    struct NavItem { const char* icon; const char* label; };
    QVector<NavItem> navItems = {
                                 {"  📊", "  Dashboard"},
                                 {"  🍴", "  Menu Management"},
                                 {"  📋", "  Orders"},
                                 {"  🪑", "  Tables"},
                                 {"  👥", "  Customers"},
                                 };

    QVector<QPushButton**> btns = {
        &m_btnDashboard, &m_btnMenu, &m_btnOrders, &m_btnTables, &m_btnCustomers
    };

    for (int i = 0; i < navItems.size(); ++i) {
        auto* btn = new QPushButton(
            QString(navItems[i].icon) + navItems[i].label, m_sidebar);
        btn->setObjectName("sidebarBtn");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(46);
        btn->setCheckable(true);
        *btns[i] = btn;
        layout->addWidget(btn);
    }

    layout->addStretch();

    // Divider
    auto* div = new QFrame(m_sidebar);
    div->setFrameShape(QFrame::HLine);
    div->setStyleSheet("background-color: #1e2332;");
    layout->addWidget(div);

    m_btnLogout = new QPushButton("  🚪  Sign Out", m_sidebar);
    m_btnLogout->setObjectName("sidebarBtn");
    m_btnLogout->setCursor(Qt::PointingHandCursor);
    m_btnLogout->setFixedHeight(46);
    m_btnLogout->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: #f87171; "
        "text-align: left; padding: 12px 16px; font-size: 14px; }"
        "QPushButton:hover { background-color: rgba(248,113,113,0.1); }");
    layout->addWidget(m_btnLogout);
    layout->addSpacing(8);
}

// ── DASHBOARD PAGE ──────────────────────────────────────────────────────────

void AdminWindow::setupDashboardPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(20);

    // Header
    auto* hdr = new QHBoxLayout();
    auto* title = new QLabel("Dashboard", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #f97316;");
    auto* dateLbl = new QLabel(QDate::currentDate().toString("dddd, MMMM d yyyy"), page);
    dateLbl->setStyleSheet("color: #8892a4; font-size: 13px;");
    auto* refreshBtn = new QPushButton("↻  Refresh", page);
    refreshBtn->setFixedWidth(110);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &AdminWindow::refreshDashboard);
    hdr->addWidget(title);
    hdr->addWidget(dateLbl);
    hdr->addStretch();
    hdr->addWidget(refreshBtn);
    layout->addLayout(hdr);

    // Stat cards
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(16);
    m_cardRevenue = createStatCard("Total Revenue", "$0.00",   "💰", "#f5a623");
    m_cardOrders  = createStatCard("Total Orders",  "0",       "📋", "#4ade80");
    m_cardToday   = createStatCard("Today's Sales", "$0.00",   "📅", "#60a5fa");
    m_cardAvg     = createStatCard("Avg Order Value","$0.00",  "📈", "#c084fc");
    statsRow->addWidget(m_cardRevenue);
    statsRow->addWidget(m_cardOrders);
    statsRow->addWidget(m_cardToday);
    statsRow->addWidget(m_cardAvg);
    layout->addLayout(statsRow);

    // Bottom section
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(16);

    // Recent orders table
    auto* recentCard = new QFrame(page);
    recentCard->setObjectName("card");
    auto* recentL = new QVBoxLayout(recentCard);
    recentL->setContentsMargins(16, 16, 16, 16);
    recentL->setSpacing(12);

    auto* recentHdr = new QHBoxLayout();
    auto* recentTitle = new QLabel("Recent Orders", recentCard);
    recentTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #f97316;");
    recentHdr->addWidget(recentTitle);
    recentL->addLayout(recentHdr);

    m_recentOrdersTable = new QTableWidget(0, 5, recentCard);
    m_recentOrdersTable->setHorizontalHeaderLabels({"Order #", "Customer", "Type", "Total", "Status"});
    m_recentOrdersTable->horizontalHeader()->setStretchLastSection(true);
    m_recentOrdersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recentOrdersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentOrdersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentOrdersTable->setAlternatingRowColors(true);
    m_recentOrdersTable->verticalHeader()->hide();
    recentL->addWidget(m_recentOrdersTable);
    bottomRow->addWidget(recentCard, 2);

    // Top items
    auto* topCard = new QFrame(page);
    topCard->setObjectName("card");
    auto* topL = new QVBoxLayout(topCard);
    topL->setContentsMargins(16, 16, 16, 16);
    topL->setSpacing(12);
    auto* topTitle = new QLabel("Top Selling Items", topCard);
    topTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #f97316;");
    topL->addWidget(topTitle);
    m_topItemsWidget = new QWidget(topCard);
    m_topItemsWidget->setLayout(new QVBoxLayout());
    m_topItemsWidget->layout()->setContentsMargins(0,0,0,0);
    topL->addWidget(m_topItemsWidget);
    topL->addStretch();
    bottomRow->addWidget(topCard, 1);

    layout->addLayout(bottomRow);
    m_pages->addWidget(page);
}

QFrame* AdminWindow::createStatCard(const QString& title, const QString& value,
                                    const QString& icon, const QString& color) {
    auto* card = new QFrame();
    card->setObjectName("statCard");
    card->setMinimumHeight(110);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(4);

    auto* topRow = new QHBoxLayout();
    auto* iconLbl = new QLabel(icon, card);
    iconLbl->setStyleSheet("font-size: 24px;");
    auto* titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet("color: #8892a4; font-size: 12px; font-weight: 600; letter-spacing: 0.5px;");
    topRow->addWidget(iconLbl);
    topRow->addStretch();
    topRow->addWidget(titleLbl);

    auto* valueLbl = new QLabel(value, card);
    valueLbl->setObjectName("cardValue");
    valueLbl->setStyleSheet(QString("font-size: 28px; font-weight: 800; color: %1;").arg(color));

    // Store color for later updates
    card->setProperty("valueColor", color);

    layout->addLayout(topRow);
    layout->addWidget(valueLbl);
    return card;
}

void AdminWindow::refreshDashboard() {
    // Update stat cards
    auto updateCard = [](QFrame* card, const QString& newVal) {
        auto* lbl = card->findChild<QLabel*>("cardValue");
        if (lbl) lbl->setText(newVal);
    };

    Database& db = Database::instance();
    updateCard(m_cardRevenue, QString("$%1").arg(db.totalRevenue(), 0, 'f', 2));
    updateCard(m_cardOrders,  QString::number(db.totalOrderCount()));
    updateCard(m_cardToday,   QString("$%1").arg(db.revenueToday(), 0, 'f', 2));
    updateCard(m_cardAvg,     QString("$%1").arg(db.averageOrderValue(), 0, 'f', 2));

    // Recent orders
    auto orders = db.recentOrders(10);
    m_recentOrdersTable->setRowCount(0);
    for (const auto& o : orders) {
        int row = m_recentOrdersTable->rowCount();
        m_recentOrdersTable->insertRow(row);
        m_recentOrdersTable->setItem(row, 0, new QTableWidgetItem(QString("#%1").arg(o.id())));
        m_recentOrdersTable->setItem(row, 1, new QTableWidgetItem(o.customerName()));
        m_recentOrdersTable->setItem(row, 2, new QTableWidgetItem(o.typeString()));
        m_recentOrdersTable->setItem(row, 3, new QTableWidgetItem(o.totalString()));

        auto* statusItem = new QTableWidgetItem(o.statusString());
        if (o.status() == Order::Status::Delivered)
            statusItem->setForeground(QColor("#4ade80"));
        else if (o.status() == Order::Status::Pending)
            statusItem->setForeground(QColor("#fbbf24"));
        else if (o.status() == Order::Status::Cancelled)
            statusItem->setForeground(QColor("#f87171"));
        else
            statusItem->setForeground(QColor("#60a5fa"));
        m_recentOrdersTable->setItem(row, 4, statusItem);
    }

    // Top items
    auto* topLayout = qobject_cast<QVBoxLayout*>(m_topItemsWidget->layout());
    QLayoutItem* child;
    while ((child = topLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    auto topItems = db.topSellingItems(5);
    int maxQty = topItems.isEmpty() ? 1 : topItems.first().second;
    QStringList colors = {"#f5a623", "#4ade80", "#60a5fa", "#c084fc", "#f87171"};

    for (int i = 0; i < topItems.size(); ++i) {
        auto* row = new QHBoxLayout();
        auto* nameLbl = new QLabel(topItems[i].first, m_topItemsWidget);
        nameLbl->setStyleSheet("color: #f97316; font-size: 12px;");
        nameLbl->setFixedWidth(160);
        nameLbl->setWordWrap(true);

        auto* bar = new QFrame(m_topItemsWidget);
        bar->setFixedHeight(8);
        int pct = maxQty > 0 ? (topItems[i].second * 100) / maxQty : 0;
        bar->setFixedWidth(pct);
        bar->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(colors[i % colors.size()]));

        auto* cntLbl = new QLabel(QString::number(topItems[i].second), m_topItemsWidget);
        cntLbl->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 700;").arg(colors[i % colors.size()]));

        auto* itemFrame = new QWidget(m_topItemsWidget);
        auto* itemLayout = new QVBoxLayout(itemFrame);
        itemLayout->setContentsMargins(0, 4, 0, 4);
        itemLayout->setSpacing(4);
        auto* barRow = new QHBoxLayout();
        barRow->addWidget(bar);
        barRow->addWidget(cntLbl);
        barRow->addStretch();
        itemLayout->addWidget(nameLbl);
        itemLayout->addLayout(barRow);
        topLayout->addWidget(itemFrame);
    }
    if (topItems.isEmpty()) {
        auto* emptyLbl = new QLabel("No sales data yet", m_topItemsWidget);
        emptyLbl->setStyleSheet("color: #3d4560; font-size: 13px;");
        emptyLbl->setAlignment(Qt::AlignCenter);
        topLayout->addWidget(emptyLbl);
    }
    topLayout->addStretch();
}

// ── MENU PAGE ───────────────────────────────────────────────────────────────

void AdminWindow::setupMenuPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    // Header
    auto* hdr = new QHBoxLayout();
    auto* title = new QLabel("Menu Management", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #f97316;");

    m_menuSearch = new QLineEdit(page);
    m_menuSearch->setPlaceholderText("🔍  Search menu items...");
    m_menuSearch->setFixedWidth(280);
    m_menuSearch->setObjectName("searchBox");

    m_menuCategoryFilter = new QComboBox(page);
    m_menuCategoryFilter->addItems(MenuItem::allCategoryNames());
    m_menuCategoryFilter->setFixedWidth(160);

    m_addItemBtn    = new QPushButton("+ Add Item", page);
    m_addItemBtn->setObjectName("primaryBtn");
    m_addItemBtn->setFixedWidth(120);
    m_addItemBtn->setCursor(Qt::PointingHandCursor);

    m_addDealBtn    = new QPushButton("+ Add Deal", page);
    m_addDealBtn->setObjectName("primaryBtn");
    m_addDealBtn->setFixedWidth(120);
    m_addDealBtn->setCursor(Qt::PointingHandCursor);

    m_editItemBtn   = new QPushButton("Edit", page);
    m_editItemBtn->setFixedWidth(90);
    m_editItemBtn->setCursor(Qt::PointingHandCursor);

    m_deleteItemBtn = new QPushButton("Delete", page);
    m_deleteItemBtn->setObjectName("dangerBtn");
    m_deleteItemBtn->setFixedWidth(100);
    m_deleteItemBtn->setCursor(Qt::PointingHandCursor);

    hdr->addWidget(title);
    hdr->addStretch();
    hdr->addWidget(m_menuSearch);
    hdr->addWidget(m_menuCategoryFilter);
    hdr->addWidget(m_addItemBtn);
    hdr->addWidget(m_addDealBtn);
    hdr->addWidget(m_editItemBtn);
    hdr->addWidget(m_deleteItemBtn);
    layout->addLayout(hdr);

    // Table
    m_menuTable = new QTableWidget(0, 7, page);
    m_menuTable->setHorizontalHeaderLabels({"ID", "Name", "Category", "Price", "Available", "Stock", "Description"});
    m_menuTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_menuTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_menuTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_menuTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_menuTable->setAlternatingRowColors(true);
    m_menuTable->verticalHeader()->hide();
    m_menuTable->setColumnWidth(0, 50);
    m_menuTable->setColumnWidth(2, 110);
    m_menuTable->setColumnWidth(3, 80);
    m_menuTable->setColumnWidth(4, 90);
    m_menuTable->setColumnWidth(5, 70);
    layout->addWidget(m_menuTable);

    m_pages->addWidget(page);
}

void AdminWindow::refreshMenuTable() {
    QString search = m_menuSearch ? m_menuSearch->text().toLower() : "";
    MenuItem::Category filterCat = m_menuCategoryFilter
                                       ? MenuItem::categoryFromString(m_menuCategoryFilter->currentText())
                                       : MenuItem::Category::All;

    m_menuItems = Database::instance().allMenuItems();
    m_menuTable->setRowCount(0);

    for (const auto& item : m_menuItems) {
        // Category filter
        if (filterCat != MenuItem::Category::All && item.category() != filterCat) continue;
        // Search filter
        if (!search.isEmpty() && !item.name().toLower().contains(search)
            && !item.description().toLower().contains(search)) continue;

        int row = m_menuTable->rowCount();
        m_menuTable->insertRow(row);
        m_menuTable->setItem(row, 0, new QTableWidgetItem(QString::number(item.id())));
        m_menuTable->setItem(row, 1, new QTableWidgetItem(item.name()));
        m_menuTable->setItem(row, 2, new QTableWidgetItem(item.categoryString()));
        m_menuTable->setItem(row, 3, new QTableWidgetItem(item.priceString()));

        auto* availItem = new QTableWidgetItem(item.isAvailable() ? "✓ Yes" : "✗ No");
        availItem->setForeground(item.isAvailable() ? QColor("#4ade80") : QColor("#f87171"));
        m_menuTable->setItem(row, 4, availItem);

        m_menuTable->setItem(row, 5, new QTableWidgetItem(QString::number(item.stock())));
        m_menuTable->setItem(row, 6, new QTableWidgetItem(item.description()));
        m_menuTable->setRowHeight(row, 44);
    }
}

bool AdminWindow::showMenuItemDialog(MenuItem& item, bool isEdit) {
    QDialog dlg(this);
    dlg.setWindowTitle(isEdit ? "Edit Menu Item" : "Add Menu Item");
    dlg.setFixedSize(500, 520);

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto* title = new QLabel(isEdit ? "Edit Menu Item" : "Add New Menu Item", &dlg);
    title->setStyleSheet("font-size: 18px; font-weight: 700; color: #f5a623;");

    auto* form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    auto* nameEdit = new QLineEdit(item.name(), &dlg);
    nameEdit->setPlaceholderText("Item name");

    auto* descEdit = new QTextEdit(&dlg);
    descEdit->setPlainText(item.description());
    descEdit->setFixedHeight(80);
    descEdit->setPlaceholderText("Short description of the dish...");

    auto* priceEdit = new QDoubleSpinBox(&dlg);
    priceEdit->setRange(0.01, 9999.99);
    priceEdit->setDecimals(2);
    priceEdit->setPrefix("$");
    priceEdit->setValue(item.price() > 0 ? item.price() : 9.99);
    priceEdit->setButtonSymbols(QAbstractSpinBox::PlusMinus);

    auto* catCombo = new QComboBox(&dlg);
    catCombo->addItems({"Appetizers", "Main Course", "Desserts", "Beverages", "Specials"});
    catCombo->setCurrentText(item.categoryString() == "All" ? "Appetizers" : item.categoryString());

    auto* stockSpin = new QSpinBox(&dlg);
    stockSpin->setRange(0, 9999);
    stockSpin->setValue(item.stock() > 0 ? item.stock() : 100);
    stockSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);

    auto* availCheck = new QCheckBox("Item is available for ordering", &dlg);
    availCheck->setChecked(item.isAvailable());

    auto* imageEdit = new QLineEdit(item.imagePath(), &dlg);
    imageEdit->setPlaceholderText("Path to image (optional)");
    auto* browseImageBtn = new QPushButton("Browse", &dlg);
    browseImageBtn->setObjectName("ghostBtn");
    auto* imageRow = new QHBoxLayout();
    imageRow->addWidget(imageEdit, 1);
    imageRow->addWidget(browseImageBtn);

    form->addRow("Name *:",        nameEdit);
    form->addRow("Description:",   descEdit);
    form->addRow("Price *:",       priceEdit);
    form->addRow("Category *:",    catCombo);
    form->addRow("Stock:",         stockSpin);
    form->addRow("Image Path:",    imageRow);
    form->addRow("",               availCheck);

    auto* errLbl = new QLabel(&dlg);
    errLbl->setStyleSheet("color: #f87171; font-size: 12px;");
    errLbl->setAlignment(Qt::AlignCenter);

    auto* btnRow = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Cancel", &dlg);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    auto* saveBtn = new QPushButton(isEdit ? "Save Changes" : "Add Item", &dlg);
    saveBtn->setObjectName("primaryBtn");
    saveBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);

    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(errLbl);
    layout->addLayout(btnRow);

    bool accepted = false;
    connect(browseImageBtn, &QPushButton::clicked, [&]() {
        const QString path = QFileDialog::getOpenFileName(
            &dlg, "Choose food image", QString(), "Images (*.png *.jpg *.jpeg *.webp *.bmp)");
        if (!path.isEmpty())
            imageEdit->setText(path);
    });
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, [&]() {
        if (nameEdit->text().trimmed().isEmpty()) {
            errLbl->setText("Item name is required.");
            return;
        }
        if (priceEdit->value() <= 0) {
            errLbl->setText("Price must be greater than 0.");
            return;
        }
        item.setName(nameEdit->text().trimmed());
        item.setDescription(descEdit->toPlainText().trimmed());
        item.setPrice(priceEdit->value());
        item.setCategory(MenuItem::categoryFromString(catCombo->currentText()));
        item.setStock(stockSpin->value());
        item.setAvailable(availCheck->isChecked());
        item.setImagePath(imageEdit->text().trimmed());
        accepted = true;
        dlg.accept();
    });

    dlg.exec();
    return accepted;
}

void AdminWindow::onAddMenuItem() {
    MenuItem item;
    if (showMenuItemDialog(item, false)) {
        if (Database::instance().addMenuItem(item)) {
            showNotification("✓  Menu item added successfully!");
            refreshMenuTable();
        } else {
            showNotification("✗  Failed: " + Database::instance().lastError(), false);
        }
    }
}

void AdminWindow::onAddDeal() {
    MenuItem item;
    item.setName("New Deal");
    item.setDescription("Bundle deal with multiple items. Edit the items and offer details before saving.");
    item.setPrice(19.99);
    item.setCategory(MenuItem::Category::Specials);
    item.setImagePath(":/images/menu/specials.jpg");

    if (showMenuItemDialog(item, false)) {
        item.setCategory(MenuItem::Category::Specials);
        if (Database::instance().addMenuItem(item)) {
            showNotification("Deal added to Specials.");
            refreshMenuTable();
        } else {
            showNotification("Failed: " + Database::instance().lastError(), false);
        }
    }
}

void AdminWindow::onEditMenuItem() {
    int row = m_menuTable->currentRow();
    if (row < 0) { showNotification("⚠  Please select an item to edit.", false); return; }

    int itemId = m_menuTable->item(row, 0)->text().toInt();
    MenuItem item = Database::instance().menuItemById(itemId);

    if (showMenuItemDialog(item, true)) {
        if (Database::instance().updateMenuItem(item)) {
            showNotification("✓  Menu item updated!");
            refreshMenuTable();
        } else {
            showNotification("✗  Update failed: " + Database::instance().lastError(), false);
        }
    }
}

void AdminWindow::onDeleteMenuItem() {
    int row = m_menuTable->currentRow();
    if (row < 0) { showNotification("⚠  Please select an item to delete.", false); return; }

    QString name = m_menuTable->item(row, 1)->text();
    int itemId = m_menuTable->item(row, 0)->text().toInt();

    auto reply = QMessageBox::question(this, "Confirm Delete",
                                       QString("Delete '%1' from the menu?\nThis cannot be undone.").arg(name),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (Database::instance().deleteMenuItem(itemId)) {
            showNotification("✓  Item deleted.");
            refreshMenuTable();
        } else {
            showNotification("✗  Delete failed.", false);
        }
    }
}

void AdminWindow::onMenuSearch(const QString&) { refreshMenuTable(); }
void AdminWindow::onCategoryFilter(int) { refreshMenuTable(); }

// ── ORDERS PAGE ─────────────────────────────────────────────────────────────

void AdminWindow::setupOrdersPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    // Header
    auto* hdr = new QHBoxLayout();
    auto* title = new QLabel("Orders", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #f97316;");

    m_orderStatusFilter = new QComboBox(page);
    m_orderStatusFilter->addItems({"All Status", "Pending", "Preparing", "Ready", "Delivered", "Cancelled"});
    m_orderStatusFilter->setFixedWidth(160);

    m_updateStatusBtn = new QPushButton("Update Status", page);
    m_updateStatusBtn->setCursor(Qt::PointingHandCursor);
    m_viewOrderBtn = new QPushButton("View Details", page);
    m_viewOrderBtn->setCursor(Qt::PointingHandCursor);

    auto* refreshBtn = new QPushButton("↻ Refresh", page);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &AdminWindow::refreshOrdersTable);

    hdr->addWidget(title);
    hdr->addStretch();
    hdr->addWidget(m_orderStatusFilter);
    hdr->addWidget(m_updateStatusBtn);
    hdr->addWidget(m_viewOrderBtn);
    hdr->addWidget(refreshBtn);
    layout->addLayout(hdr);

    m_ordersTable = new QTableWidget(0, 8, page);
    m_ordersTable->setHorizontalHeaderLabels(
        {"Order #", "Customer", "Type", "Table/Addr", "Items", "Total", "Status", "Time"});
    m_ordersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ordersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ordersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ordersTable->setAlternatingRowColors(true);
    m_ordersTable->verticalHeader()->hide();
    layout->addWidget(m_ordersTable);

    m_pages->addWidget(page);
}

void AdminWindow::refreshOrdersTable() {
    m_orders = Database::instance().allOrders();
    QString statusFilter = m_orderStatusFilter->currentText();
    m_ordersTable->setRowCount(0);

    for (const auto& o : m_orders) {
        if (statusFilter != "All Status" && o.statusString() != statusFilter) continue;

        int row = m_ordersTable->rowCount();
        m_ordersTable->insertRow(row);
        m_ordersTable->setItem(row, 0, new QTableWidgetItem(QString("#%1").arg(o.id())));
        m_ordersTable->setItem(row, 1, new QTableWidgetItem(o.customerName()));
        m_ordersTable->setItem(row, 2, new QTableWidgetItem(o.typeString()));

        QString loc;
        if (o.type() == Order::Type::DineIn) {
            Restaurant rest;
            QString area = "Dining Area";
            for (const auto& table : rest.allTables()) {
                if (table.number == o.tableNumber()) {
                    area = table.section;
                    break;
                }
            }
            loc = QString("Table %1 - %2").arg(o.tableNumber()).arg(area);
        }
        else if (o.type() == Order::Type::Delivery) loc = o.deliveryAddr().left(30) + "...";
        else loc = "Takeaway";
        m_ordersTable->setItem(row, 3, new QTableWidgetItem(loc));

        m_ordersTable->setItem(row, 4, new QTableWidgetItem(QString::number(o.itemCount())));
        m_ordersTable->setItem(row, 5, new QTableWidgetItem(o.totalString()));

        auto* statusItem = new QTableWidgetItem(o.statusString());
        if (o.status() == Order::Status::Delivered)
            statusItem->setForeground(QColor("#4ade80"));
        else if (o.status() == Order::Status::Pending)
            statusItem->setForeground(QColor("#fbbf24"));
        else if (o.status() == Order::Status::Cancelled)
            statusItem->setForeground(QColor("#f87171"));
        else if (o.status() == Order::Status::Preparing)
            statusItem->setForeground(QColor("#60a5fa"));
        else
            statusItem->setForeground(QColor("#c084fc"));
        m_ordersTable->setItem(row, 6, statusItem);

        m_ordersTable->setItem(row, 7, new QTableWidgetItem(
                                           o.timestamp().toString("MMM d, hh:mm")));
        m_ordersTable->setRowHeight(row, 44);
    }
}

void AdminWindow::onUpdateOrderStatus() {
    int row = m_ordersTable->currentRow();
    if (row < 0) { showNotification("⚠  Select an order first.", false); return; }

    // Get order ID from display
    QString orderIdStr = m_ordersTable->item(row, 0)->text();
    orderIdStr.remove('#');
    int orderId = orderIdStr.toInt();

    QDialog dlg(this);
    dlg.setWindowTitle("Update Order Status");
    dlg.setFixedSize(340, 200);

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto* lbl = new QLabel(QString("Update status for Order #%1").arg(orderId), &dlg);
    lbl->setStyleSheet("font-size: 14px; font-weight: 600; color: #f97316;");

    auto* combo = new QComboBox(&dlg);
    combo->addItems({"Pending", "Preparing", "Ready", "Delivered", "Cancelled"});

    auto* btnRow = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Cancel", &dlg);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    auto* updateBtn = new QPushButton("Update", &dlg);
    updateBtn->setObjectName("primaryBtn");
    updateBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(updateBtn);

    layout->addWidget(lbl);
    layout->addWidget(combo);
    layout->addStretch();
    layout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(updateBtn, &QPushButton::clicked, [&]() {
        Order::Status newStatus = Order::statusFromString(combo->currentText());
        if (Database::instance().updateOrderStatus(orderId, newStatus)) {
            showNotification("✓  Order status updated!");
            refreshOrdersTable();
        } else {
            showNotification("✗  Update failed.", false);
        }
        dlg.accept();
    });
    dlg.exec();
}

void AdminWindow::onViewOrderDetails() {
    int row = m_ordersTable->currentRow();
    if (row < 0) { showNotification("⚠  Select an order first.", false); return; }

    QString orderIdStr = m_ordersTable->item(row, 0)->text();
    orderIdStr.remove('#');
    int orderId = orderIdStr.toInt();
    Order o = Database::instance().orderById(orderId);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Order #%1 Details").arg(orderId));
    dlg.setFixedSize(500, 520);

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto* hdr = new QLabel(QString("Order #%1 — %2").arg(o.id()).arg(o.typeString()), &dlg);
    hdr->setStyleSheet("font-size: 18px; font-weight: 700; color: #f5a623;");

    QString orderInfo = QString("Customer: %1\nStatus: %2\nTime: %3\n%4")
            .arg(o.customerName(), o.statusString(),
                 o.timestamp().toString("MMM d yyyy, hh:mm"),
                 o.type() == Order::Type::DineIn
                     ? QString("Table: %1").arg(o.tableNumber())
                     : o.type() == Order::Type::Delivery
                           ? "Address: " + o.deliveryAddr()
                           : "Takeaway");
    if (!o.notes().isEmpty())
        orderInfo += "\nNotes: " + o.notes();

    auto* info = new QLabel(orderInfo, &dlg);
    info->setStyleSheet("color: #8892a4; font-size: 13px; line-height: 160%;");

    auto* itemsTable = new QTableWidget(0, 3, &dlg);
    itemsTable->setHorizontalHeaderLabels({"Item", "Qty", "Subtotal"});
    itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    itemsTable->verticalHeader()->hide();
    itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (const auto& oi : o.items()) {
        int r = itemsTable->rowCount();
        itemsTable->insertRow(r);
        itemsTable->setItem(r, 0, new QTableWidgetItem(oi.item.name()));
        itemsTable->setItem(r, 1, new QTableWidgetItem(QString("x%1").arg(oi.quantity)));
        itemsTable->setItem(r, 2, new QTableWidgetItem(
                                      QString("$%1").arg(oi.subtotal(), 0, 'f', 2)));
    }

    auto* totalsLbl = new QLabel(
        QString("Subtotal: $%1\nTax (%2%): $%3\nDiscount (%4%): -$%5\n\nGrand Total: %6")
            .arg(o.subtotal(), 0, 'f', 2)
            .arg(o.taxRate(), 0, 'f', 1)
            .arg(o.taxAmount(), 0, 'f', 2)
            .arg(o.discount(), 0, 'f', 1)
            .arg(o.discountAmt(), 0, 'f', 2)
            .arg(o.totalString()), &dlg);
    totalsLbl->setStyleSheet("color: #f97316; font-size: 13px; line-height: 160%;");
    totalsLbl->setAlignment(Qt::AlignRight);

    auto* closeBtn = new QPushButton("Close", &dlg);
    closeBtn->setObjectName("primaryBtn");
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    layout->addWidget(hdr);
    layout->addWidget(info);
    layout->addWidget(itemsTable);
    layout->addWidget(totalsLbl);
    layout->addWidget(closeBtn);
    dlg.exec();
}

// ── TABLES PAGE ─────────────────────────────────────────────────────────────

void AdminWindow::setupTablesPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    auto* hdr = new QHBoxLayout();
    auto* title = new QLabel("Table Management", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #f97316;");
    auto* refreshBtn = new QPushButton("↻ Refresh", page);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &AdminWindow::refreshTableGrid);
    hdr->addWidget(title);
    hdr->addStretch();
    hdr->addWidget(refreshBtn);
    layout->addLayout(hdr);

    // Legend
    auto* legend = new QHBoxLayout();
    auto* freeLbl = new QLabel("⬜ Free", page);
    freeLbl->setStyleSheet("color: #4ade80; font-size: 13px; font-weight: 600;");
    auto* occLbl = new QLabel("⬛ Occupied", page);
    occLbl->setStyleSheet("color: #f87171; font-size: 13px; font-weight: 600;");
    legend->addWidget(freeLbl);
    legend->addSpacing(16);
    legend->addWidget(occLbl);
    legend->addStretch();
    layout->addLayout(legend);

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    m_tablesGrid = new QWidget();
    m_tablesGrid->setLayout(new QGridLayout());
    qobject_cast<QGridLayout*>(m_tablesGrid->layout())->setSpacing(12);
    qobject_cast<QGridLayout*>(m_tablesGrid->layout())->setContentsMargins(0, 0, 0, 0);

    scroll->setWidget(m_tablesGrid);
    layout->addWidget(scroll);

    m_pages->addWidget(page);
}

void AdminWindow::refreshTableGrid() {
    auto* grid = qobject_cast<QGridLayout*>(m_tablesGrid->layout());

    // Clear existing
    QLayoutItem* child;
    while ((child = grid->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    Restaurant rest;
    auto tables = rest.allTables();
    QSet<int> occupiedByOrders;
    for (const auto& order : Database::instance().allOrders()) {
        if (order.type() == Order::Type::DineIn
            && order.status() != Order::Status::Delivered
            && order.status() != Order::Status::Cancelled
            && order.tableNumber() > 0) {
            occupiedByOrders.insert(order.tableNumber());
        }
    }
    int col = 0, row = 0;

    QString lastSection = "";
    for (const auto& t : tables) {
        if (t.section != lastSection) {
            auto* sectionLbl = new QLabel(t.section, m_tablesGrid);
            sectionLbl->setStyleSheet(
                "font-size: 13px; font-weight: 700; color: #8892a4; "
                "letter-spacing: 1px; text-transform: uppercase;");
            if (col > 0) { row++; col = 0; }
            grid->addWidget(sectionLbl, row++, 0, 1, 6);
            lastSection = t.section;
        }

        const bool visuallyOccupied = occupiedByOrders.contains(t.number)
                                      || QRandomGenerator::global()->bounded(100) < 42;

        auto* card = new QFrame(m_tablesGrid);
        card->setFixedSize(120, 100);
        card->setStyleSheet(
            visuallyOccupied
                ? "background-color: rgba(248,113,113,0.1); border: 2px solid #f87171; border-radius: 12px;"
                : "background-color: rgba(74,222,128,0.1); border: 2px solid #4ade80; border-radius: 12px;");

        auto* cLayout = new QVBoxLayout(card);
        cLayout->setAlignment(Qt::AlignCenter);

        auto* numLbl = new QLabel(QString("Table %1").arg(t.number), card);
        numLbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #f97316;");
        numLbl->setAlignment(Qt::AlignCenter);

        auto* capLbl = new QLabel(QString("%1 seats").arg(t.capacity), card);
        capLbl->setStyleSheet("font-size: 11px; color: #8892a4;");
        capLbl->setAlignment(Qt::AlignCenter);

        auto* areaLbl = new QLabel(t.section, card);
        areaLbl->setStyleSheet("font-size: 10px; color: #f97316; font-weight: 700;");
        areaLbl->setAlignment(Qt::AlignCenter);

        auto* statusLbl = new QLabel(visuallyOccupied ? "OCCUPIED" : "FREE", card);
        statusLbl->setStyleSheet(visuallyOccupied
                                     ? "font-size: 10px; font-weight: 700; color: #f87171; letter-spacing: 0.5px;"
                                     : "font-size: 10px; font-weight: 700; color: #4ade80; letter-spacing: 0.5px;");
        statusLbl->setAlignment(Qt::AlignCenter);

        cLayout->addWidget(numLbl);
        cLayout->addWidget(capLbl);
        cLayout->addWidget(areaLbl);
        cLayout->addWidget(statusLbl);

        grid->addWidget(card, row, col % 6);
        col++;
        if (col % 6 == 0) { row++; col = 0; }
    }
}

// ── CUSTOMERS PAGE ──────────────────────────────────────────────────────────

void AdminWindow::setupCustomersPage() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    auto* hdr = new QHBoxLayout();
    auto* title = new QLabel("Customer Management", page);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #f97316;");
    auto* refreshBtn = new QPushButton("↻ Refresh", page);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    hdr->addWidget(title);
    hdr->addStretch();
    hdr->addWidget(refreshBtn);
    layout->addLayout(hdr);

    auto* custTable = new QTableWidget(0, 6, page);
    custTable->setHorizontalHeaderLabels({"ID", "Name", "Email", "Phone", "Loyalty Pts", "Status"});
    custTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    custTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    custTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    custTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    custTable->setAlternatingRowColors(true);
    custTable->verticalHeader()->hide();
    layout->addWidget(custTable);

    auto loadCustomers = [custTable]() {
        custTable->setRowCount(0);
        auto customers = Database::instance().allCustomers();
        for (const auto& c : customers) {
            int row = custTable->rowCount();
            custTable->insertRow(row);
            custTable->setItem(row, 0, new QTableWidgetItem(QString::number(c.id())));
            custTable->setItem(row, 1, new QTableWidgetItem(c.name()));
            custTable->setItem(row, 2, new QTableWidgetItem(c.email()));
            custTable->setItem(row, 3, new QTableWidgetItem(c.phone()));
            custTable->setItem(row, 4, new QTableWidgetItem(QString::number(c.loyaltyPts())));
            auto* si = new QTableWidgetItem(c.isActive() ? "Active" : "Inactive");
            si->setForeground(c.isActive() ? QColor("#4ade80") : QColor("#f87171"));
            custTable->setItem(row, 5, si);
            custTable->setRowHeight(row, 44);
        }
    };

    connect(refreshBtn, &QPushButton::clicked, loadCustomers);
    loadCustomers();

    m_pages->addWidget(page);
}

// ── NAVIGATION ───────────────────────────────────────────────────────────────

void AdminWindow::navigateTo(int page) {
    m_currentPage = page;
    m_pages->setCurrentIndex(page);

    QVector<QPushButton*> btns = {
        m_btnDashboard, m_btnMenu, m_btnOrders, m_btnTables, m_btnCustomers
    };
    for (int i = 0; i < btns.size(); ++i) {
        btns[i]->setChecked(i == page);
        btns[i]->setObjectName(i == page ? "sidebarBtnActive" : "sidebarBtn");
        btns[i]->style()->unpolish(btns[i]);
        btns[i]->style()->polish(btns[i]);
    }

    switch (page) {
    case 0: refreshDashboard();   break;
    case 1: refreshMenuTable();   break;
    case 2: refreshOrdersTable(); break;
    case 3: refreshTableGrid();   break;
    default: break;
    }
}

void AdminWindow::setupConnections() {
    connect(m_btnDashboard,  &QPushButton::clicked, [this]{ navigateTo(0); });
    connect(m_btnMenu,       &QPushButton::clicked, [this]{ navigateTo(1); });
    connect(m_btnOrders,     &QPushButton::clicked, [this]{ navigateTo(2); });
    connect(m_btnTables,     &QPushButton::clicked, [this]{ navigateTo(3); });
    connect(m_btnCustomers,  &QPushButton::clicked, [this]{ navigateTo(4); });
    connect(m_btnLogout,     &QPushButton::clicked, this, &AdminWindow::onLogout);

    connect(m_addItemBtn,    &QPushButton::clicked, this, &AdminWindow::onAddMenuItem);
    connect(m_addDealBtn,    &QPushButton::clicked, this, &AdminWindow::onAddDeal);
    connect(m_editItemBtn,   &QPushButton::clicked, this, &AdminWindow::onEditMenuItem);
    connect(m_deleteItemBtn, &QPushButton::clicked, this, &AdminWindow::onDeleteMenuItem);
    connect(m_menuSearch,    &QLineEdit::textChanged, this, &AdminWindow::onMenuSearch);
    connect(m_menuCategoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdminWindow::onCategoryFilter);

    connect(m_updateStatusBtn, &QPushButton::clicked, this, &AdminWindow::onUpdateOrderStatus);
    connect(m_viewOrderBtn,    &QPushButton::clicked, this, &AdminWindow::onViewOrderDetails);
    connect(m_orderStatusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int){ refreshOrdersTable(); });
}

void AdminWindow::onLogout() {
    auto reply = QMessageBox::question(this, "Sign Out",
                                       "Are you sure you want to sign out?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
        emit logoutRequested();
}

void AdminWindow::showNotification(const QString& msg, bool success) {
    m_notification->setText(msg);
    m_notification->setStyleSheet(
        success
            ? "background-color: #166534; color: #4ade80; border-radius: 10px; padding: 10px 20px; font-size: 13px; font-weight: 600;"
            : "background-color: #7f1d1d; color: #fca5a5; border-radius: 10px; padding: 10px 20px; font-size: 13px; font-weight: 600;");
    m_notification->adjustSize();
    m_notification->move(width() - m_notification->width() - 20, height() - m_notification->height() - 20);
    m_notification->show();
    m_notification->raise();
    m_notifTimer->start(3000);
}

void AdminWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    if (m_notification && m_notification->isVisible()) {
        m_notification->move(width() - m_notification->width() - 20,
                             height() - m_notification->height() - 20);
    }
}
