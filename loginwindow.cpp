#include "loginwindow.h"

#include "database.h"

#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QVBoxLayout>

class ImageModeButton : public QPushButton {
public:
    ImageModeButton(const QString& title, const QString& headline,
                    const QString& body, const QString& imagePath,
                    QWidget* parent = nullptr)
        : QPushButton(parent),
          m_title(title),
          m_headline(headline),
          m_body(body),
          m_pixmap(imagePath)
    {
        setText(title + "\n" + headline + "\n" + body);
        setMinimumHeight(190);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF buttonRect = rect().adjusted(1, 1, -1, -1);
        QPainterPath clipPath;
        clipPath.addRoundedRect(buttonRect, 22, 22);
        p.setClipPath(clipPath);

        if (!m_pixmap.isNull()) {
            const QSize targetSize = buttonRect.size().toSize();
            QPixmap scaled = m_pixmap.scaled(targetSize, Qt::KeepAspectRatioByExpanding,
                                             Qt::SmoothTransformation);
            const QPoint topLeft(buttonRect.left() + (buttonRect.width() - scaled.width()) / 2,
                                 buttonRect.top() + (buttonRect.height() - scaled.height()) / 2);
            p.drawPixmap(topLeft, scaled);
        } else {
            p.fillPath(clipPath, QColor("#fff7ed"));
        }

        const QRectF labelBand(buttonRect.left(), buttonRect.bottom() - 76,
                               buttonRect.width(), 76);
        QLinearGradient shade(labelBand.topLeft(), labelBand.bottomLeft());
        shade.setColorAt(0.0, QColor(0, 0, 0, 15));
        shade.setColorAt(0.25, QColor(0, 0, 0, 100));
        shade.setColorAt(1.0, QColor(0, 0, 0, 185));
        p.fillRect(labelBand, shade);

        p.setClipping(false);
        p.setPen(QPen(underMouse() ? QColor("#f97316") : QColor("#fed7aa"), 2));
        p.drawRoundedRect(buttonRect, 22, 22);

        const QRect textRect = rect().adjusted(14, height() - 72, -14, -12);
        p.setPen(Qt::white);
        QFont titleFont("Segoe UI", 10, QFont::Black);
        p.setFont(titleFont);
        p.drawText(textRect.adjusted(0, 0, 0, -42), Qt::AlignCenter, m_title);

        QFont headlineFont("Segoe UI", 12, QFont::Black);
        p.setFont(headlineFont);
        p.drawText(textRect.adjusted(0, 18, 0, -21), Qt::AlignCenter | Qt::TextWordWrap, m_headline);

        QFont bodyFont("Segoe UI", 8, QFont::DemiBold);
        p.setFont(bodyFont);
        p.drawText(textRect.adjusted(0, 45, 0, 0), Qt::AlignCenter | Qt::TextWordWrap, m_body);
    }

private:
    QString m_title;
    QString m_headline;
    QString m_body;
    QPixmap m_pixmap;
};

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent),
      m_logoAnim(0)
{
    setWindowTitle("Corrindor - Restaurant System");
    setMinimumSize(760, 680);
    resize(920, 720);

    QRect geo = QApplication::primaryScreen()->geometry();
    move((geo.width() - width()) / 2, (geo.height() - height()) / 2);

    setupUi();
    setupConnections();

    m_logoTimer = new QTimer(this);
    connect(m_logoTimer, &QTimer::timeout, this, &LoginWindow::animateLogo);
    m_logoTimer->start(450);
}

void LoginWindow::setupUi() {
    setObjectName("loginPage");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* bg = new QFrame(this);
    bg->setObjectName("welcomePage");
    auto* bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(44, 34, 44, 26);
    bgLayout->setSpacing(18);
    root->addWidget(bg);

    auto* brandFrame = new QFrame(bg);
    brandFrame->setObjectName("welcomeBrandFrame");
    auto* brandLayout = new QVBoxLayout(brandFrame);
    brandLayout->setAlignment(Qt::AlignCenter);
    brandLayout->setSpacing(8);

    m_logoLabel = new QLabel("Corrindor", brandFrame);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setObjectName("welcomeLogo");
    auto* restaurantName = new QLabel("Corrindor Kitchen", brandFrame);
    restaurantName->setAlignment(Qt::AlignCenter);
    restaurantName->setObjectName("welcomeTitle");
    auto* tagline = new QLabel("Choose how you want to order today", brandFrame);
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setObjectName("welcomeSubtitle");

    brandLayout->addWidget(m_logoLabel);
    brandLayout->addWidget(restaurantName);
    brandLayout->addWidget(tagline);
    bgLayout->addWidget(brandFrame);

    auto createModeCard = [bg](const QString& title, const QString& headline,
                               const QString& body, const QString& imagePath) {
        auto* btn = new ImageModeButton(title, headline, body, imagePath, bg);
        btn->setObjectName("modeCard");
        return btn;
    };

    auto* modeGrid = new QGridLayout();
    modeGrid->setHorizontalSpacing(16);
    modeGrid->setVerticalSpacing(16);
    m_dineInBtn = createModeCard("DINE IN", "Eat at a table",
                                 "Confirm a table before ordering",
                                 ":/images/welcome/dine-table.jpg");
    m_deliveryBtn = createModeCard("DELIVERY", "Bring it to me",
                                   "Track a delivery bike on the way",
                                   ":/images/welcome/custom-delivery-bike.jpg");
    m_takeawayBtn = createModeCard("TAKEAWAY", "Pick it up",
                                   "Food bag packed with the Corrindor logo",
                                   ":/images/welcome/food-bag-logo.jpg");
    m_adminBtn = createModeCard("ADMIN", "Staff sign in",
                                "Open the admin login area",
                                ":/images/welcome/staff.jpg");
    modeGrid->addWidget(m_dineInBtn, 0, 0);
    modeGrid->addWidget(m_deliveryBtn, 0, 1);
    modeGrid->addWidget(m_takeawayBtn, 1, 0);
    modeGrid->addWidget(m_adminBtn, 1, 1);
    bgLayout->addLayout(modeGrid);

    bgLayout->addStretch();

    m_errorLabel = new QLabel("", bg);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setObjectName("loginError");
    m_errorLabel->hide();
    bgLayout->addWidget(m_errorLabel);

    auto* footer = new QLabel("Guests choose an order type. Staff use the Admin card.", bg);
    footer->setAlignment(Qt::AlignCenter);
    footer->setObjectName("welcomeFooter");
    bgLayout->addWidget(footer);

    m_errorTimer = new QTimer(this);
    m_errorTimer->setSingleShot(true);
    connect(m_errorTimer, &QTimer::timeout, this, &LoginWindow::clearError);
}

void LoginWindow::setupConnections() {
    connect(m_dineInBtn, &QPushButton::clicked, this, [this] {
        emit orderModeSelected(Order::Type::DineIn);
    });
    connect(m_deliveryBtn, &QPushButton::clicked, this, [this] {
        emit orderModeSelected(Order::Type::Delivery);
    });
    connect(m_takeawayBtn, &QPushButton::clicked, this, [this] {
        emit orderModeSelected(Order::Type::Takeaway);
    });
    connect(m_adminBtn, &QPushButton::clicked, this, &LoginWindow::showAdminLoginDialog);
}

void LoginWindow::onAdminLogin() {
    showAdminLoginDialog();
}

bool LoginWindow::validateAdminLogin(const QString& username, const QString& password,
                                     Admin& admin, QString& error) const {
    if (username.isEmpty() || password.isEmpty()) {
        error = "Please enter both username and password.";
        return false;
    }

    if (!Database::instance().adminExists(username)) {
        error = "Username not found. Please check your credentials.";
        return false;
    }

    admin = Database::instance().adminByUsername(username);
    if (!admin.isActive()) {
        error = "This account has been deactivated.";
        return false;
    }

    if (!Admin::verifyPassword(password, admin.passwordHash())) {
        error = "Incorrect password. Please try again.";
        return false;
    }

    return true;
}

void LoginWindow::showAdminLoginDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Admin login");
    dialog.setModal(true);
    dialog.setFixedSize(440, 300);
    dialog.setObjectName("adminLoginDialog");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(12);

    auto* title = new QLabel("Admin Login", &dialog);
    title->setObjectName("dialogTitle");
    title->setAlignment(Qt::AlignCenter);

    auto* hint = new QLabel("Default: admin / admin123", &dialog);
    hint->setObjectName("dialogDetails");
    hint->setAlignment(Qt::AlignCenter);

    auto* username = new QLineEdit(&dialog);
    username->setPlaceholderText("Username");
    username->setText("admin");
    username->setMinimumHeight(42);

    auto* password = new QLineEdit(&dialog);
    password->setPlaceholderText("Password");
    password->setEchoMode(QLineEdit::Password);
    password->setText("admin123");
    password->setMinimumHeight(42);

    auto* error = new QLabel(&dialog);
    error->setObjectName("loginError");
    error->setAlignment(Qt::AlignCenter);
    error->hide();

    auto* buttons = new QHBoxLayout();
    auto* cancel = new QPushButton("Cancel", &dialog);
    cancel->setObjectName("ghostBtn");
    auto* signIn = new QPushButton("Sign in", &dialog);
    signIn->setObjectName("primaryBtn");
    signIn->setMinimumHeight(40);
    buttons->addWidget(cancel);
    buttons->addWidget(signIn);

    layout->addWidget(title);
    layout->addWidget(hint);
    layout->addWidget(username);
    layout->addWidget(password);
    layout->addWidget(error);
    layout->addStretch();
    layout->addLayout(buttons);

    auto tryLogin = [&]() {
        Admin admin;
        QString errorText;
        if (!validateAdminLogin(username->text().trimmed(), password->text(), admin, errorText)) {
            error->setText(errorText);
            error->show();
            return;
        }
        dialog.accept();
        emit adminLoggedIn(admin);
    };

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(signIn, &QPushButton::clicked, &dialog, tryLogin);
    connect(password, &QLineEdit::returnPressed, &dialog, tryLogin);

    username->setFocus();
    username->selectAll();
    dialog.exec();
}

void LoginWindow::showError(const QString& msg) {
    m_errorLabel->setText(msg);
    m_errorLabel->show();
    m_errorTimer->start(4000);
}

void LoginWindow::clearError() {
    m_errorLabel->hide();
    m_errorLabel->clear();
}

void LoginWindow::animateLogo() {
    const QStringList labels = {
        "C", "Co", "Cor", "Corr", "Corri",
        "Corrin", "Corrind", "Corrindo", "Corrindor"
    };
    m_logoAnim = (m_logoAnim + 1) % labels.size();
    m_logoLabel->setText(labels[m_logoAnim]);
}
