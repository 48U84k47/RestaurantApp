#include "customerwindow.h"

#include "database.h"
#include "restaurant.h"

#include <QApplication>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSet>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

class DeliveryMapWidget : public QWidget {
public:
    explicit DeliveryMapWidget(QWidget* parent = nullptr)
        : QWidget(parent), m_progress(0)
    {
        setMinimumHeight(180);
        setObjectName("deliveryMap");
    }

    void setProgress(int progress) {
        m_progress = qBound(0, progress, 100);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF r = rect().adjusted(10, 10, -10, -10);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#fff7ed"));
        p.drawRoundedRect(r, 12, 12);

        QPen roadPen(QColor("#fed7aa"), 16, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QPainterPath route;
        const QPointF start(r.left() + 34, r.bottom() - 36);
        const QPointF mid1(r.left() + r.width() * 0.36, r.top() + 42);
        const QPointF mid2(r.left() + r.width() * 0.67, r.bottom() - 56);
        const QPointF end(r.right() - 34, r.top() + 36);
        route.moveTo(start);
        route.cubicTo(mid1, mid2, end);
        p.setPen(roadPen);
        p.drawPath(route);

        p.setPen(QPen(QColor("#fb923c"), 4, Qt::DashLine, Qt::RoundCap));
        p.drawPath(route);

        drawPin(p, start, QColor("#f97316"), "C");
        drawPin(p, end, QColor("#1f2937"), "H");

        const qreal t = m_progress / 100.0;
        QPointF rider = route.pointAtPercent(t);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f97316"));
        p.drawEllipse(rider, 12, 12);
        p.setBrush(Qt::white);
        p.drawEllipse(rider + QPointF(4, 4), 3, 3);
    }

private:
    void drawPin(QPainter& p, const QPointF& pos, const QColor& color, const QString& text) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(pos, 16, 16);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 10, QFont::Bold));
        p.drawText(QRectF(pos.x() - 16, pos.y() - 16, 32, 32), Qt::AlignCenter, text);
    }

    int m_progress;
};

CustomerWindow::CustomerWindow(const Customer& customer, bool guestMode, QWidget* parent,
                               Order::Type initialType, int initialTableNumber,
                               int initialPartySize)
    : QWidget(parent),
      m_customer(customer),
      m_guestMode(guestMode),
      m_dineInTableAssigned(initialType == Order::Type::DineIn && initialTableNumber > 0),
      m_previousOrderType(initialType),
      m_deliveryProgress(0)
{
    m_cart.setCustomerId(guestMode ? -1 : customer.id());
    m_cart.setCustomerName(displayCustomerName());

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* left = new QFrame(this);
    left->setObjectName("customerMain");
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(28, 22, 24, 22);
    leftLayout->setSpacing(16);

    auto* topBar = new QHBoxLayout();
    auto* brand = new QLabel("Corrindor", left);
    brand->setObjectName("customerBrand");
    auto* user = new QLabel("Hi, " + displayCustomerName(), left);
    user->setObjectName("customerUser");
    auto* logoutButton = new QPushButton("Log out", left);
    logoutButton->setObjectName("ghostBtn");
    topBar->addWidget(brand);
    topBar->addStretch();
    topBar->addWidget(user);
    topBar->addWidget(logoutButton);
    leftLayout->addLayout(topBar);

    auto* hero = new QFrame(left);
    hero->setObjectName("foodHero");
    auto* heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(22, 18, 22, 18);
    auto* heroText = new QVBoxLayout();
    auto* heroTitle = new QLabel("Fast Italian delivery", hero);
    heroTitle->setObjectName("heroTitle");
    auto* heroSub = new QLabel("Order pasta, desserts, drinks, and chef specials in a simple delivery flow.", hero);
    heroSub->setObjectName("heroSub");
    auto* heroMeta = new QLabel("4.8 rating  |  25-35 min  |  Free service on dine-in", hero);
    heroMeta->setObjectName("heroMeta");
    heroText->addWidget(heroTitle);
    heroText->addWidget(heroSub);
    heroText->addWidget(heroMeta);
    auto* heroBadge = new QLabel("HOT\nDEALS", hero);
    heroBadge->setObjectName("heroBadge");
    heroBadge->setAlignment(Qt::AlignCenter);
    heroLayout->addLayout(heroText, 1);
    heroLayout->addWidget(heroBadge);
    leftLayout->addWidget(hero);

    auto* filters = new QHBoxLayout();
    m_searchEdit = new QLineEdit(left);
    m_searchEdit->setObjectName("foodSearch");
    m_searchEdit->setPlaceholderText("Search for pizza, pasta, drinks...");
    m_searchEdit->setMinimumHeight(42);

    m_categoryFilter = new QComboBox(left);
    m_categoryFilter->setObjectName("categoryPicker");
    m_categoryFilter->addItems(MenuItem::allCategoryNames());
    m_categoryFilter->setMinimumHeight(42);
    m_categoryFilter->setFixedWidth(180);

    filters->addWidget(m_searchEdit);
    filters->addWidget(m_categoryFilter);
    leftLayout->addLayout(filters);

    auto* sectionTitle = new QLabel("Popular dishes", left);
    sectionTitle->setObjectName("sectionTitle");
    leftLayout->addWidget(sectionTitle);

    m_menuScroll = new QScrollArea(left);
    m_menuScroll->setWidgetResizable(true);
    m_menuScroll->setFrameShape(QFrame::NoFrame);
    m_menuScroll->setObjectName("menuScroll");

    m_menuContainer = new QWidget(m_menuScroll);
    m_menuGrid = new QGridLayout(m_menuContainer);
    m_menuGrid->setContentsMargins(0, 0, 8, 0);
    m_menuGrid->setHorizontalSpacing(14);
    m_menuGrid->setVerticalSpacing(14);
    m_menuScroll->setWidget(m_menuContainer);
    leftLayout->addWidget(m_menuScroll, 1);

    auto* right = new QFrame(this);
    right->setObjectName("checkoutPanel");
    right->setFixedWidth(390);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(20, 22, 20, 22);
    rightLayout->setSpacing(12);

    auto* cartTitle = new QLabel("Your basket", right);
    cartTitle->setObjectName("cartTitle");
    rightLayout->addWidget(cartTitle);

    m_statusLabel = new QLabel("Add items to start your order.", right);
    m_statusLabel->setObjectName("softText");
    m_statusLabel->setWordWrap(true);
    rightLayout->addWidget(m_statusLabel);

    m_orderType = new QComboBox(right);
    m_orderType->setObjectName("checkoutInput");
    m_orderType->addItems({"Dine-In", "Takeaway", "Delivery"});
    m_orderType->setCurrentText(Order::typeToString(initialType));
    rightLayout->addWidget(m_orderType);

    m_tableNumber = new QSpinBox(right);
    m_tableNumber->setRange(1, 30);
    m_tableNumber->setPrefix("Table ");
    m_tableNumber->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    rightLayout->addWidget(m_tableNumber);

    m_tableAreaLabel = new QLabel("Area will be assigned after guest count.", right);
    m_tableAreaLabel->setObjectName("tableAreaLabel");
    m_tableAreaLabel->setWordWrap(true);
    rightLayout->addWidget(m_tableAreaLabel);

    m_partySize = new QSpinBox(right);
    m_partySize->setRange(1, 30);
    m_partySize->setPrefix("Guests ");
    m_partySize->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    rightLayout->addWidget(m_partySize);

    m_addressEdit = new QLineEdit(right);
    m_addressEdit->setPlaceholderText("Delivery address / area");
    m_addressEdit->hide();
    rightLayout->addWidget(m_addressEdit);

    auto* deliveryFlow = new QLabel("1 Address  |  2 Basket  |  3 Confirm  |  4 Track rider", right);
    deliveryFlow->setObjectName("deliveryFlow");
    deliveryFlow->setWordWrap(true);
    rightLayout->addWidget(deliveryFlow);

    m_notesEdit = new QTextEdit(right);
    m_notesEdit->setPlaceholderText("Add cooking notes");
    m_notesEdit->setFixedHeight(64);
    rightLayout->addWidget(m_notesEdit);

    m_cartList = new QListWidget(right);
    m_cartList->setObjectName("basketList");
    m_cartList->setMinimumHeight(160);
    rightLayout->addWidget(m_cartList, 1);

    auto* cartButtons = new QHBoxLayout();
    auto* removeButton = new QPushButton("Remove", right);
    removeButton->setObjectName("ghostBtn");
    auto* clearButton = new QPushButton("Clear", right);
    clearButton->setObjectName("ghostBtn");
    cartButtons->addWidget(removeButton);
    cartButtons->addWidget(clearButton);
    rightLayout->addLayout(cartButtons);

    m_subtotalLabel = new QLabel(right);
    m_taxLabel = new QLabel(right);
    m_deliveryFeeLabel = new QLabel(right);
    m_totalLabel = new QLabel(right);
    m_totalLabel->setObjectName("totalLabel");
    rightLayout->addWidget(m_subtotalLabel);
    rightLayout->addWidget(m_taxLabel);
    rightLayout->addWidget(m_deliveryFeeLabel);
    rightLayout->addWidget(m_totalLabel);

    m_placeOrderButton = new QPushButton("Place order", right);
    m_placeOrderButton->setObjectName("primaryBtn");
    m_placeOrderButton->setMinimumHeight(46);
    rightLayout->addWidget(m_placeOrderButton);

    auto* trackerTitle = new QLabel("Delivery tracker", right);
    trackerTitle->setObjectName("trackerTitle");
    rightLayout->addWidget(trackerTitle);

    m_trackingStatusLabel = new QLabel("Place a delivery order to start live tracking.", right);
    m_trackingStatusLabel->setObjectName("softText");
    m_trackingStatusLabel->setWordWrap(true);
    rightLayout->addWidget(m_trackingStatusLabel);

    m_mapWidget = new DeliveryMapWidget(right);
    rightLayout->addWidget(m_mapWidget);

    m_etaLabel = new QLabel("ETA --", right);
    m_etaLabel->setObjectName("etaLabel");
    rightLayout->addWidget(m_etaLabel);

    root->addWidget(left, 1);
    root->addWidget(right);

    m_deliveryTimer = new QTimer(this);
    m_deliveryTimer->setInterval(900);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CustomerWindow::refreshMenu);
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomerWindow::refreshMenu);
    connect(m_orderType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomerWindow::onOrderTypeChanged);
    connect(m_tableNumber, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CustomerWindow::updateCart);
    connect(m_tableNumber, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CustomerWindow::updateTableAreaLabel);
    connect(m_partySize, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CustomerWindow::updateCart);
    connect(removeButton, &QPushButton::clicked, this, &CustomerWindow::removeSelectedCartItem);
    connect(clearButton, &QPushButton::clicked, this, &CustomerWindow::clearCart);
    connect(m_placeOrderButton, &QPushButton::clicked, this, &CustomerWindow::placeOrder);
    connect(logoutButton, &QPushButton::clicked, this, &CustomerWindow::logoutRequested);
    connect(m_deliveryTimer, &QTimer::timeout, this, &CustomerWindow::advanceDelivery);

    if (initialType == Order::Type::DineIn && initialTableNumber > 0) {
        m_tableNumber->setValue(initialTableNumber);
        m_partySize->setValue(qMax(1, initialPartySize));
        updateTableAreaLabel();
    }
    onOrderTypeChanged(m_orderType->currentIndex());
    refreshMenu();
    updateCart();
}

QString CustomerWindow::displayCustomerName() const {
    if (m_guestMode)
        return "Guest";
    return m_customer.displayName().isEmpty() ? "Customer" : m_customer.displayName();
}

QFrame* CustomerWindow::createMenuCard(const MenuItem& item) {
    auto* card = new QFrame(m_menuContainer);
    card->setObjectName("foodCard");
    card->setMinimumHeight(240);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 14);
    layout->setSpacing(10);

    auto* image = new QLabel(card);
    image->setObjectName("dishImage");
    image->setFixedHeight(118);
    image->setScaledContents(false);
    image->setAlignment(Qt::AlignCenter);

    QPixmap pix(resolvedImagePath(item));
    if (!pix.isNull()) {
        image->setPixmap(pix.scaledToWidth(360, Qt::SmoothTransformation));
    } else {
        image->setText(item.categoryString());
    }

    auto* imageOverlay = new QWidget(image);
    imageOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    imageOverlay->setGeometry(image->rect());
    auto* imageLayout = new QHBoxLayout(imageOverlay);
    imageLayout->setContentsMargins(12, 8, 12, 8);
    auto* tag = new QLabel(item.categoryString(), imageOverlay);
    tag->setObjectName("dishTag");
    imageLayout->addWidget(tag);
    imageLayout->addStretch();
    auto* price = new QLabel(item.priceString(), imageOverlay);
    price->setObjectName("dishPrice");
    imageLayout->addWidget(price);

    auto* content = new QVBoxLayout();
    content->setContentsMargins(14, 0, 14, 0);
    auto* name = new QLabel(item.name(), card);
    name->setObjectName("dishName");
    name->setWordWrap(true);
    auto* desc = new QLabel(item.description(), card);
    desc->setObjectName("dishDesc");
    desc->setWordWrap(true);
    auto* meta = new QLabel("4.7 rating  |  15 min prep", card);
    meta->setObjectName("dishMeta");
    content->addWidget(name);
    content->addWidget(desc);
    content->addWidget(meta);

    auto* bottom = new QHBoxLayout();
    bottom->setContentsMargins(14, 0, 14, 0);
    auto* stock = new QLabel(item.isAvailable() ? "Available now" : "Sold out", card);
    stock->setObjectName(item.isAvailable() ? "availableLabel" : "soldOutLabel");
    auto* addButton = new QPushButton(item.isAvailable() ? "Add +" : "Sold out", card);
    addButton->setObjectName(item.isAvailable() ? "primaryBtn" : "disabledBtn");
    addButton->setEnabled(item.isAvailable());
    addButton->setFixedWidth(92);
    addButton->setMinimumHeight(34);
    connect(addButton, &QPushButton::clicked, this, [this, item]() {
        addItemToCart(item.id());
    });
    bottom->addWidget(stock);
    bottom->addStretch();
    bottom->addWidget(addButton);

    layout->addWidget(image);
    layout->addLayout(content);
    layout->addStretch();
    layout->addLayout(bottom);
    return card;
}

void CustomerWindow::clearMenuGrid() {
    QLayoutItem* child;
    while ((child = m_menuGrid->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
}

void CustomerWindow::refreshMenu() {
    clearMenuGrid();

    m_menuItems = Database::instance().allMenuItems();
    const QString search = m_searchEdit->text().trimmed().toLower();
    const auto category = MenuItem::categoryFromString(m_categoryFilter->currentText());

    int visibleCount = 0;
    for (const auto& item : m_menuItems) {
        if (category != MenuItem::Category::All && item.category() != category)
            continue;
        if (!search.isEmpty()
            && !item.name().toLower().contains(search)
            && !item.description().toLower().contains(search))
            continue;

        m_menuGrid->addWidget(createMenuCard(item), visibleCount / 2, visibleCount % 2);
        visibleCount++;
    }

    if (visibleCount == 0) {
        auto* empty = new QLabel("No dishes match your search.", m_menuContainer);
        empty->setAlignment(Qt::AlignCenter);
        empty->setObjectName("emptyState");
        m_menuGrid->addWidget(empty, 0, 0, 1, 2);
    }

    m_menuGrid->setRowStretch((visibleCount + 1) / 2, 1);
}

void CustomerWindow::addItemToCart(int menuItemId) {
    for (const auto& item : m_menuItems) {
        if (item.id() == menuItemId) {
            m_cart.addItem(item, 1);
            playClickSound();
            updateCart();
            return;
        }
    }
}

void CustomerWindow::updateCart() {
    m_cart.setType(selectedOrderType());
    m_cart.setTableNumber(m_tableNumber->value());
    m_cart.setDeliveryAddress(m_addressEdit->text().trimmed());

    QString notes = m_notesEdit->toPlainText().trimmed();
    if (selectedOrderType() == Order::Type::DineIn) {
        const QString partyNote = QString("Party size: %1").arg(m_partySize->value());
        const QString areaNote = QString("Table area: %1").arg(tableAreaForNumber(m_tableNumber->value()));
        const QString tableNote = partyNote + "\n" + areaNote;
        notes = notes.isEmpty() ? tableNote : tableNote + "\n" + notes;
    }
    m_cart.setNotes(notes);

    m_cartList->clear();
    for (const auto& line : m_cart.items()) {
        auto* row = new QListWidgetItem(
            QString("%1\n%2 x %3")
                .arg(line.item.name())
                .arg(line.quantity)
                .arg(QString("$%1").arg(line.subtotal(), 0, 'f', 2)),
            m_cartList);
        row->setData(Qt::UserRole, line.item.id());
    }

    const double deliveryFee = selectedOrderType() == Order::Type::Delivery && !m_cart.isEmpty() ? 2.99 : 0.0;
    const double displayTotal = m_cart.grandTotal() + deliveryFee;
    m_subtotalLabel->setText(QString("Subtotal: $%1").arg(m_cart.subtotal(), 0, 'f', 2));
    m_taxLabel->setText(QString("Tax: $%1").arg(m_cart.taxAmount(), 0, 'f', 2));
    m_deliveryFeeLabel->setText(QString("Delivery fee: $%1").arg(deliveryFee, 0, 'f', 2));
    m_totalLabel->setText(QString("Total: $%1").arg(displayTotal, 0, 'f', 2));
    m_statusLabel->setText(m_cart.isEmpty()
                               ? "Add items to start your order."
                               : QString("%1 item(s) in your basket.").arg(m_cart.itemCount()));
    m_placeOrderButton->setEnabled(!m_cart.isEmpty());
}

void CustomerWindow::removeSelectedCartItem() {
    auto* selected = m_cartList->currentItem();
    if (!selected)
        return;
    m_cart.removeItem(selected->data(Qt::UserRole).toInt());
    playClickSound();
    updateCart();
}

void CustomerWindow::clearCart() {
    m_cart.clearItems();
    playClickSound();
    updateCart();
}

Order::Type CustomerWindow::selectedOrderType() const {
    return Order::typeFromString(m_orderType->currentText());
}

void CustomerWindow::onOrderTypeChanged(int) {
    const bool delivery = selectedOrderType() == Order::Type::Delivery;
    const bool dineIn = selectedOrderType() == Order::Type::DineIn;
    if (dineIn && !m_dineInTableAssigned) {
        if (!promptForDineInGuests()) {
            m_orderType->blockSignals(true);
            m_orderType->setCurrentText(Order::typeToString(m_previousOrderType));
            m_orderType->blockSignals(false);
            return;
        }
    }
    if (!dineIn)
        m_dineInTableAssigned = false;
    m_previousOrderType = selectedOrderType();
    m_addressEdit->setVisible(delivery);
    m_tableNumber->setVisible(dineIn);
    m_tableAreaLabel->setVisible(dineIn);
    m_partySize->setVisible(dineIn);
    updateTableAreaLabel();
    updateCart();
}

void CustomerWindow::placeOrder() {
    updateCart();

    if (m_cart.isEmpty()) {
        QApplication::beep();
        QMessageBox::warning(this, "Empty Basket", "Please add at least one item first.");
        return;
    }

    if (selectedOrderType() == Order::Type::Delivery && m_addressEdit->text().trimmed().isEmpty()) {
        QApplication::beep();
        QMessageBox::warning(this, "Delivery Address", "Please enter a delivery address.");
        return;
    }

    if (selectedOrderType() == Order::Type::DineIn && !m_dineInTableAssigned) {
        if (!promptForDineInGuests())
            return;
        updateCart();
    }

    Order order = m_cart;
    if (Database::instance().saveOrder(order)) {
        playSuccessSound();
        showOrderPlacedDialog(order);
        startDeliveryTracking(order.id());
        m_cart.clearItems();
        updateCart();
    } else {
        QApplication::beep();
        QMessageBox::critical(this, "Order Failed", Database::instance().lastError());
    }
}

void CustomerWindow::startDeliveryTracking(int orderId) {
    m_deliveryProgress = 0;
    m_mapWidget->setProgress(0);

    if (selectedOrderType() == Order::Type::Delivery) {
        m_trackingStatusLabel->setText(QString("Order #%1 confirmed. Restaurant accepted your order.").arg(orderId));
        m_etaLabel->setText("ETA 25 min");
        m_deliveryTimer->start();
    } else {
        m_deliveryTimer->stop();
        m_trackingStatusLabel->setText(QString("Order #%1 sent to the kitchen. We will call you when it is ready.").arg(orderId));
        m_etaLabel->setText(selectedOrderType() == Order::Type::DineIn
                                ? QString("Table %1, %2").arg(m_tableNumber->value()).arg(tableAreaForNumber(m_tableNumber->value()))
                                : "Pickup in 20 min");
        m_mapWidget->setProgress(100);
    }
}

void CustomerWindow::showOrderPlacedDialog(const Order& order) const {
    QDialog dialog(const_cast<CustomerWindow*>(this));
    dialog.setWindowTitle("Order placed");
    dialog.setModal(true);
    dialog.setFixedSize(430, 320);
    dialog.setObjectName("orderDialog");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto* badge = new QLabel("ORDER CONFIRMED", &dialog);
    badge->setObjectName("dialogBadge");
    badge->setAlignment(Qt::AlignCenter);

    auto* title = new QLabel(QString("Order #%1 is on the way").arg(order.id()), &dialog);
    title->setObjectName("dialogTitle");
    title->setAlignment(Qt::AlignCenter);

    auto* details = new QLabel(
        QString("%1\n%2 items  |  %3")
            .arg(order.typeString())
            .arg(order.itemCount())
            .arg(order.totalString()),
        &dialog);
    details->setObjectName("dialogDetails");
    details->setAlignment(Qt::AlignCenter);

    auto* progress = new QFrame(&dialog);
    progress->setObjectName("dialogProgress");
    progress->setMinimumHeight(54);
    auto* progressLayout = new QHBoxLayout(progress);
    progressLayout->setContentsMargins(14, 8, 14, 8);
    auto* step1 = new QLabel("Kitchen", progress);
    auto* step2 = new QLabel("Rider", progress);
    auto* step3 = new QLabel("Enjoy", progress);
    step1->setObjectName("dialogStepActive");
    step2->setObjectName("dialogStep");
    step3->setObjectName("dialogStep");
    step1->setAlignment(Qt::AlignCenter);
    step2->setAlignment(Qt::AlignCenter);
    step3->setAlignment(Qt::AlignCenter);
    progressLayout->addWidget(step1);
    progressLayout->addWidget(step2);
    progressLayout->addWidget(step3);

    auto* ok = new QPushButton("Track order", &dialog);
    ok->setObjectName("primaryBtn");
    ok->setMinimumHeight(42);
    connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    layout->addWidget(badge);
    layout->addWidget(title);
    layout->addWidget(details);
    layout->addWidget(progress);
    layout->addStretch();
    layout->addWidget(ok);

    dialog.exec();
}

bool CustomerWindow::promptForDineInGuests() {
    QDialog dialog(this);
    dialog.setWindowTitle("Dine-in table");
    dialog.setModal(true);
    dialog.setFixedSize(420, 260);
    dialog.setObjectName("orderDialog");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 24);
    layout->setSpacing(14);

    auto* title = new QLabel("How many guests?", &dialog);
    title->setObjectName("dialogTitle");
    title->setAlignment(Qt::AlignCenter);

    auto* detail = new QLabel("We will assign the smallest available table that fits your party.", &dialog);
    detail->setObjectName("dialogDetails");
    detail->setWordWrap(true);
    detail->setAlignment(Qt::AlignCenter);

    auto* guests = new QSpinBox(&dialog);
    guests->setRange(1, 18);
    guests->setValue(qMax(1, m_partySize->value()));
    guests->setPrefix("Guests ");
    guests->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    guests->setMinimumHeight(42);

    auto* assigned = new QLabel(&dialog);
    assigned->setObjectName("dialogBadge");
    assigned->setAlignment(Qt::AlignCenter);

    auto updateAssigned = [&]() {
        const int table = allocateTableForGuests(guests->value());
        assigned->setText(table > 0
                              ? QString("Table %1 - %2 is available").arg(table).arg(tableAreaForNumber(table))
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
        if (table <= 0) {
            QMessageBox::warning(&dialog, "No Table", "No table is available for this number of guests right now.");
            return;
        }
        m_partySize->setValue(guests->value());
        m_tableNumber->setValue(table);
        updateTableAreaLabel();
        m_dineInTableAssigned = true;
        dialog.accept();
    });

    return dialog.exec() == QDialog::Accepted;
}

int CustomerWindow::allocateTableForGuests(int guests) const {
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

QString CustomerWindow::tableAreaForNumber(int tableNumber) const {
    Restaurant rest;
    for (const auto& table : rest.allTables()) {
        if (table.number == tableNumber)
            return table.section;
    }
    return "Dining Area";
}

void CustomerWindow::updateTableAreaLabel() {
    if (!m_tableAreaLabel)
        return;
    const QString area = tableAreaForNumber(m_tableNumber->value());
    m_tableAreaLabel->setText(QString("Assigned area: %1").arg(area));
}

QString CustomerWindow::resolvedImagePath(const MenuItem& item) const {
    QString path = item.imagePath().trimmed();
    if (path.isEmpty()) {
        switch (item.category()) {
        case MenuItem::Category::Appetizers: path = ":/images/menu/appetizers.jpg"; break;
        case MenuItem::Category::MainCourse: path = ":/images/menu/main-course.jpg"; break;
        case MenuItem::Category::Desserts: path = ":/images/menu/desserts.jpg"; break;
        case MenuItem::Category::Beverages: path = ":/images/menu/beverages.jpg"; break;
        case MenuItem::Category::Specials: path = ":/images/menu/specials.jpg"; break;
        default: path = ":/images/menu/main-course.jpg"; break;
        }
    }

    if (path.startsWith(":/") || QFileInfo(path).isAbsolute())
        return path;

    const QString appPath = QCoreApplication::applicationDirPath() + "/" + path;
    if (QFileInfo::exists(appPath))
        return appPath;

    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../" + path);
}

void CustomerWindow::advanceDelivery() {
    m_deliveryProgress += 8;
    m_mapWidget->setProgress(m_deliveryProgress);

    const int eta = qMax(0, 25 - (m_deliveryProgress / 4));
    m_etaLabel->setText(QString("ETA %1 min").arg(eta));

    if (m_deliveryProgress < 30)
        m_trackingStatusLabel->setText("Restaurant is preparing your food.");
    else if (m_deliveryProgress < 65)
        m_trackingStatusLabel->setText("Rider assigned and heading to Corrindor.");
    else if (m_deliveryProgress < 100)
        m_trackingStatusLabel->setText("Food picked up. Rider is on the way.");
    else {
        m_deliveryTimer->stop();
        m_deliveryProgress = 100;
        m_mapWidget->setProgress(100);
        m_trackingStatusLabel->setText("Delivered. Enjoy your meal!");
        m_etaLabel->setText("Delivered");
        playSuccessSound();
    }
}

void CustomerWindow::playClickSound() const {
    QApplication::beep();
}

void CustomerWindow::playSuccessSound() const {
    QApplication::beep();
    QTimer::singleShot(120, []() { QApplication::beep(); });
}
