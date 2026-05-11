#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>
#include <QDateTime>

Database& Database::instance() {
    static Database inst;
    return inst;
}

Database::Database(QObject* parent) : QObject(parent) {}

bool Database::initialize(const QString& dbPath) {
    // Use a named connection so re-initialisation after logout doesn't
    // produce a "connection already exists" warning or silently reuse a
    // stale, closed handle.
    const QString connName = "corrindor_main";

    if (QSqlDatabase::contains(connName)) {
        // Already open from a previous session — just reuse it.
        m_db = QSqlDatabase::database(connName);
        if (m_db.isOpen()) return true;
        // Was closed somehow — try to re-open.
        if (!m_db.open()) {
            m_lastError = m_db.lastError().text();
            emit databaseError(m_lastError);
            return false;
        }
        return true;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        emit databaseError(m_lastError);
        return false;
    }

    // Enable WAL mode for better concurrent read performance
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");

    return createTables() && seedDefaultData();
}

bool Database::isOpen() const {
    return m_db.isOpen();
}

void Database::close() {
    m_db.close();
}

bool Database::createTables() {
    QSqlQuery q(m_db);

    // Admins table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS admins (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT    UNIQUE NOT NULL,
            password    TEXT    NOT NULL,
            full_name   TEXT    DEFAULT '',
            role        TEXT    DEFAULT 'Cashier',
            active      INTEGER DEFAULT 1,
            created_at  TEXT    DEFAULT (datetime('now'))
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Customers table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS customers (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            name            TEXT    NOT NULL,
            email           TEXT    UNIQUE NOT NULL,
            phone           TEXT    DEFAULT '',
            password        TEXT    NOT NULL,
            loyalty_points  INTEGER DEFAULT 0,
            address         TEXT    DEFAULT '',
            active          INTEGER DEFAULT 1,
            created_at      TEXT    DEFAULT (datetime('now'))
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Menu items table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS menu_items (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT    NOT NULL,
            description TEXT    DEFAULT '',
            price       REAL    NOT NULL,
            category    TEXT    NOT NULL,
            available   INTEGER DEFAULT 1,
            image_path  TEXT    DEFAULT '',
            stock       INTEGER DEFAULT 100,
            created_at  TEXT    DEFAULT (datetime('now'))
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Orders table
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS orders (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            customer_id     INTEGER,
            customer_name   TEXT    DEFAULT 'Guest',
            type            TEXT    NOT NULL,
            status          TEXT    DEFAULT 'Pending',
            table_number    INTEGER DEFAULT 0,
            delivery_addr   TEXT    DEFAULT '',
            discount        REAL    DEFAULT 0.0,
            tax_rate        REAL    DEFAULT 8.0,
            notes           TEXT    DEFAULT '',
            created_at      TEXT    DEFAULT (datetime('now')),
            FOREIGN KEY(customer_id) REFERENCES customers(id)
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Order items (line items)
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS order_items (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id    INTEGER NOT NULL,
            menu_item_id INTEGER NOT NULL,
            item_name   TEXT    NOT NULL,
            item_price  REAL    NOT NULL,
            quantity    INTEGER NOT NULL DEFAULT 1,
            FOREIGN KEY(order_id) REFERENCES orders(id)
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    return true;
}

bool Database::seedDefaultData() {
    // Only seed if tables are empty
    QSqlQuery check(m_db);
    check.exec("SELECT COUNT(*) FROM admins");
    if (check.next() && check.value(0).toInt() > 0) return true;

    // Insert default admin
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO admins (username, password, full_name, role) VALUES (?,?,?,?)");
    q.addBindValue("admin");
    q.addBindValue(Admin::hashPassword("admin123"));
    q.addBindValue("Sophia Marchetti");
    q.addBindValue("SuperAdmin");
    q.exec();

    q.prepare("INSERT INTO admins (username, password, full_name, role) VALUES (?,?,?,?)");
    q.addBindValue("manager");
    q.addBindValue(Admin::hashPassword("manager123"));
    q.addBindValue("Marco Rossi");
    q.addBindValue("Manager");
    q.exec();

    // Insert default customer
    q.prepare("INSERT INTO customers (name, email, phone, password) VALUES (?,?,?,?)");
    q.addBindValue("Isabella Romano");
    q.addBindValue("customer@corrindor.com");
    q.addBindValue("+1 555-0100");
    q.addBindValue(Customer::hashPassword("customer123"));
    q.exec();

    // Insert sample menu items
    struct SeedItem {
        const char* name; const char* desc; double price; const char* cat;
    };

    QVector<SeedItem> items = {
                               // Appetizers
                               {"Bruschetta al Pomodoro", "Grilled bread with fresh tomatoes, basil and extra virgin olive oil", 8.99, "Appetizers"},
                               {"Arancini di Riso", "Crispy Sicilian rice balls filled with mozzarella and ragù", 11.99, "Appetizers"},
                               {"Carpaccio di Manzo", "Thinly sliced prime beef with arugula, capers and parmesan shavings", 14.99, "Appetizers"},
                               {"Calamari Fritti", "Golden fried calamari served with marinara dipping sauce", 12.99, "Appetizers"},

                               // Main Course
                               {"Osso Buco alla Milanese", "Braised veal shanks with gremolata and saffron risotto", 32.99, "Main Course"},
                               {"Tagliatelle al Ragù", "Hand-rolled pasta ribbons with slow-cooked Bolognese sauce", 19.99, "Main Course"},
                               {"Branzino al Forno", "Oven-baked sea bass with Mediterranean herbs and lemon", 28.99, "Main Course"},
                               {"Bistecca Fiorentina", "Premium T-bone steak grilled over wood fire, serves 2", 54.99, "Main Course"},
                               {"Pappardelle ai Funghi", "Wide pasta ribbons with wild mushrooms, truffle oil and pecorino", 22.99, "Main Course"},
                               {"Vitello Saltimbocca", "Veal cutlets with prosciutto, sage and white wine sauce", 29.99, "Main Course"},

                               // Desserts
                               {"Tiramisu Classico", "House-made classic with mascarpone, espresso and savoiardi", 8.99, "Desserts"},
                               {"Panna Cotta", "Vanilla bean panna cotta with fresh berry coulis", 7.99, "Desserts"},
                               {"Cannoli Siciliani", "Crispy pastry shells filled with sweetened ricotta and pistachios", 9.99, "Desserts"},
                               {"Gelato Artigianale", "Three scoops of house-churned artisan gelato", 6.99, "Desserts"},

                               // Beverages
                               {"Acqua Minerale", "Still or sparkling mineral water (500ml)", 3.99, "Beverages"},
                               {"Espresso Italiano", "Double shot of our house blend espresso", 3.49, "Beverages"},
                               {"Aperol Spritz", "Aperol, Prosecco, soda water with orange slice", 9.99, "Beverages"},
                               {"Chianti Classico", "Glass of Tuscan Chianti Classico DOCG", 11.99, "Beverages"},
                               {"Limonata Artigianale", "House-squeezed lemon soda with fresh mint", 5.49, "Beverages"},

                               // Specials
                               {"Chef's Tasting Menu", "5-course curated experience by Chef Marchetti (per person)", 89.99, "Specials"},
                               {"Weekend Brunch Board", "Assorted Italian breads, cold cuts, cheeses and preserves", 24.99, "Specials"},
                               };

    for (const auto& item : items) {
        q.prepare("INSERT INTO menu_items (name, description, price, category) VALUES (?,?,?,?)");
        q.addBindValue(item.name);
        q.addBindValue(item.desc);
        q.addBindValue(item.price);
        q.addBindValue(item.cat);
        q.exec();
    }

    return true;
}

// ── MENU ITEMS ─────────────────────────────────────────────────────────────

bool Database::addMenuItem(MenuItem& item) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO menu_items (name, description, price, category, available, image_path, stock) "
              "VALUES (?,?,?,?,?,?,?)");
    q.addBindValue(item.name());
    q.addBindValue(item.description());
    q.addBindValue(item.price());
    q.addBindValue(MenuItem::categoryToString(item.category()));
    q.addBindValue(item.isAvailable() ? 1 : 0);
    q.addBindValue(item.imagePath());
    q.addBindValue(item.stock());

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    item.setId(q.lastInsertId().toInt());
    return true;
}

bool Database::updateMenuItem(const MenuItem& item) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE menu_items SET name=?, description=?, price=?, category=?, "
              "available=?, image_path=?, stock=? WHERE id=?");
    q.addBindValue(item.name());
    q.addBindValue(item.description());
    q.addBindValue(item.price());
    q.addBindValue(MenuItem::categoryToString(item.category()));
    q.addBindValue(item.isAvailable() ? 1 : 0);
    q.addBindValue(item.imagePath());
    q.addBindValue(item.stock());
    q.addBindValue(item.id());

    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool Database::deleteMenuItem(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM menu_items WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QVector<MenuItem> Database::allMenuItems() const {
    QVector<MenuItem> result;
    QSqlQuery q("SELECT id,name,description,price,category,available,image_path,stock FROM menu_items ORDER BY category,name", m_db);
    while (q.next()) {
        MenuItem item;
        item.setId(q.value(0).toInt());
        item.setName(q.value(1).toString());
        item.setDescription(q.value(2).toString());
        item.setPrice(q.value(3).toDouble());
        item.setCategory(MenuItem::categoryFromString(q.value(4).toString()));
        item.setAvailable(q.value(5).toInt() == 1);
        item.setImagePath(q.value(6).toString());
        item.setStock(q.value(7).toInt());
        result.append(item);
    }
    return result;
}

QVector<MenuItem> Database::menuItemsByCategory(MenuItem::Category cat) const {
    QVector<MenuItem> all = allMenuItems();
    if (cat == MenuItem::Category::All) return all;
    QVector<MenuItem> result;
    for (const auto& item : all) {
        if (item.category() == cat) result.append(item);
    }
    return result;
}

MenuItem Database::menuItemById(int id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT id,name,description,price,category,available,image_path,stock FROM menu_items WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        MenuItem item;
        item.setId(q.value(0).toInt());
        item.setName(q.value(1).toString());
        item.setDescription(q.value(2).toString());
        item.setPrice(q.value(3).toDouble());
        item.setCategory(MenuItem::categoryFromString(q.value(4).toString()));
        item.setAvailable(q.value(5).toInt() == 1);
        item.setImagePath(q.value(6).toString());
        item.setStock(q.value(7).toInt());
        return item;
    }
    return MenuItem();
}

bool Database::menuItemExists(int id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM menu_items WHERE id=?");
    q.addBindValue(id);
    q.exec();
    return q.next() && q.value(0).toInt() > 0;
}

// ── ORDERS ─────────────────────────────────────────────────────────────────

bool Database::saveOrder(Order& order) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO orders (customer_id, customer_name, type, status, table_number, "
              "delivery_addr, discount, tax_rate, notes) VALUES (?,?,?,?,?,?,?,?,?)");
    q.addBindValue(order.customerId() >= 0 ? order.customerId() : QVariant());
    q.addBindValue(order.customerName());
    q.addBindValue(Order::typeToString(order.type()));
    q.addBindValue(Order::statusToString(order.status()));
    q.addBindValue(order.tableNumber());
    q.addBindValue(order.deliveryAddr());
    q.addBindValue(order.discount());
    q.addBindValue(order.taxRate());
    q.addBindValue(order.notes());

    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    order.setId(q.lastInsertId().toInt());

    // Save line items
    for (const auto& oi : order.items()) {
        QSqlQuery iq(m_db);
        iq.prepare("INSERT INTO order_items (order_id, menu_item_id, item_name, item_price, quantity) VALUES (?,?,?,?,?)");
        iq.addBindValue(order.id());
        iq.addBindValue(oi.item.id());
        iq.addBindValue(oi.item.name());
        iq.addBindValue(oi.item.price());
        iq.addBindValue(oi.quantity);
        if (!iq.exec()) { m_lastError = iq.lastError().text(); return false; }
    }
    return true;
}

bool Database::updateOrderStatus(int orderId, Order::Status status) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE orders SET status=? WHERE id=?");
    q.addBindValue(Order::statusToString(status));
    q.addBindValue(orderId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

void Database::loadOrderItems(Order& order) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT menu_item_id, item_name, item_price, quantity FROM order_items WHERE order_id=?");
    q.addBindValue(order.id());
    q.exec();
    while (q.next()) {
        MenuItem item;
        item.setId(q.value(0).toInt());
        item.setName(q.value(1).toString());
        item.setPrice(q.value(2).toDouble());
        order.addItem(item, q.value(3).toInt());
    }
}

QVector<Order> Database::allOrders() const {
    QVector<Order> result;
    QSqlQuery q("SELECT id, customer_id, customer_name, type, status, table_number, "
                "delivery_addr, discount, tax_rate, notes, created_at "
                "FROM orders ORDER BY created_at DESC", m_db);
    while (q.next()) {
        Order o;
        o.setId(q.value(0).toInt());
        o.setCustomerId(q.value(1).isNull() ? -1 : q.value(1).toInt());
        o.setCustomerName(q.value(2).toString());
        o.setType(Order::typeFromString(q.value(3).toString()));
        o.setStatus(Order::statusFromString(q.value(4).toString()));
        o.setTableNumber(q.value(5).toInt());
        o.setDeliveryAddress(q.value(6).toString());
        o.setDiscount(q.value(7).toDouble());
        o.setTaxRate(q.value(8).toDouble());
        o.setNotes(q.value(9).toString());
        o.setTimestamp(QDateTime::fromString(q.value(10).toString(), "yyyy-MM-dd HH:mm:ss"));
        if (!o.timestamp().isValid())   // fallback for any ISO-T format stored by older code
            o.setTimestamp(QDateTime::fromString(q.value(10).toString(), Qt::ISODate));
        loadOrderItems(o);
        result.append(o);
    }
    return result;
}

QVector<Order> Database::ordersByCustomer(int customerId) const {
    QVector<Order> all = allOrders();
    QVector<Order> result;
    for (const auto& o : all)
        if (o.customerId() == customerId) result.append(o);
    return result;
}

QVector<Order> Database::ordersByStatus(Order::Status status) const {
    QVector<Order> all = allOrders();
    QVector<Order> result;
    for (const auto& o : all)
        if (o.status() == status) result.append(o);
    return result;
}

QVector<Order> Database::recentOrders(int limit) const {
    QVector<Order> all = allOrders();
    if (all.size() <= limit) return all;
    return all.mid(0, limit);
}

Order Database::orderById(int id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT id, customer_id, customer_name, type, status, table_number, "
              "delivery_addr, discount, tax_rate, notes, created_at FROM orders WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        Order o;
        o.setId(q.value(0).toInt());
        o.setCustomerId(q.value(1).isNull() ? -1 : q.value(1).toInt());
        o.setCustomerName(q.value(2).toString());
        o.setType(Order::typeFromString(q.value(3).toString()));
        o.setStatus(Order::statusFromString(q.value(4).toString()));
        o.setTableNumber(q.value(5).toInt());
        o.setDeliveryAddress(q.value(6).toString());
        o.setDiscount(q.value(7).toDouble());
        o.setTaxRate(q.value(8).toDouble());
        o.setNotes(q.value(9).toString());
        o.setTimestamp(QDateTime::fromString(q.value(10).toString(), "yyyy-MM-dd HH:mm:ss"));
        if (!o.timestamp().isValid())
            o.setTimestamp(QDateTime::fromString(q.value(10).toString(), Qt::ISODate));
        loadOrderItems(o);
        return o;
    }
    return Order();
}

// ── CUSTOMERS ──────────────────────────────────────────────────────────────

bool Database::addCustomer(Customer& customer) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO customers (name, email, phone, password, loyalty_points, address) VALUES (?,?,?,?,?,?)");
    q.addBindValue(customer.name());
    q.addBindValue(customer.email());
    q.addBindValue(customer.phone());
    q.addBindValue(customer.passwordHash());
    q.addBindValue(customer.loyaltyPts());
    q.addBindValue(customer.address());
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    customer.setId(q.lastInsertId().toInt());
    return true;
}

bool Database::updateCustomer(const Customer& customer) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE customers SET name=?, email=?, phone=?, loyalty_points=?, address=?, active=? WHERE id=?");
    q.addBindValue(customer.name());
    q.addBindValue(customer.email());
    q.addBindValue(customer.phone());
    q.addBindValue(customer.loyaltyPts());
    q.addBindValue(customer.address());
    q.addBindValue(customer.isActive() ? 1 : 0);
    q.addBindValue(customer.id());
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QVector<Customer> Database::allCustomers() const {
    QVector<Customer> result;
    QSqlQuery q("SELECT id,name,email,phone,password,loyalty_points,address,active FROM customers ORDER BY name", m_db);
    while (q.next()) {
        Customer c;
        c.setId(q.value(0).toInt());
        c.setName(q.value(1).toString());
        c.setEmail(q.value(2).toString());
        c.setPhone(q.value(3).toString());
        c.setPasswordHash(q.value(4).toString());
        c.setLoyaltyPoints(q.value(5).toInt());
        c.setAddress(q.value(6).toString());
        c.setActive(q.value(7).toInt() == 1);
        result.append(c);
    }
    return result;
}

Customer Database::customerByEmail(const QString& email) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT id,name,email,phone,password,loyalty_points,address,active FROM customers WHERE email=?");
    q.addBindValue(email);
    q.exec();
    if (q.next()) {
        Customer c;
        c.setId(q.value(0).toInt());
        c.setName(q.value(1).toString());
        c.setEmail(q.value(2).toString());
        c.setPhone(q.value(3).toString());
        c.setPasswordHash(q.value(4).toString());
        c.setLoyaltyPoints(q.value(5).toInt());
        c.setAddress(q.value(6).toString());
        c.setActive(q.value(7).toInt() == 1);
        return c;
    }
    return Customer();
}

Customer Database::customerById(int id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT id,name,email,phone,password,loyalty_points,address,active FROM customers WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        Customer c;
        c.setId(q.value(0).toInt());
        c.setName(q.value(1).toString());
        c.setEmail(q.value(2).toString());
        c.setPhone(q.value(3).toString());
        c.setPasswordHash(q.value(4).toString());
        c.setLoyaltyPoints(q.value(5).toInt());
        c.setAddress(q.value(6).toString());
        c.setActive(q.value(7).toInt() == 1);
        return c;
    }
    return Customer();
}

bool Database::customerExists(const QString& email) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM customers WHERE email=?");
    q.addBindValue(email);
    q.exec();
    return q.next() && q.value(0).toInt() > 0;
}

// ── ADMINS ─────────────────────────────────────────────────────────────────

bool Database::addAdmin(Admin& admin) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO admins (username, password, full_name, role) VALUES (?,?,?,?)");
    q.addBindValue(admin.username());
    q.addBindValue(admin.passwordHash());
    q.addBindValue(admin.fullName());
    q.addBindValue(Admin::roleToString(admin.role()));
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    admin.setId(q.lastInsertId().toInt());
    return true;
}

bool Database::updateAdmin(const Admin& admin) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE admins SET username=?, full_name=?, role=?, active=? WHERE id=?");
    q.addBindValue(admin.username());
    q.addBindValue(admin.fullName());
    q.addBindValue(Admin::roleToString(admin.role()));
    q.addBindValue(admin.isActive() ? 1 : 0);
    q.addBindValue(admin.id());
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

QVector<Admin> Database::allAdmins() const {
    QVector<Admin> result;
    QSqlQuery q("SELECT id,username,password,full_name,role,active FROM admins", m_db);
    while (q.next()) {
        Admin a;
        a.setId(q.value(0).toInt());
        a.setUsername(q.value(1).toString());
        a.setPasswordHash(q.value(2).toString());
        a.setFullName(q.value(3).toString());
        a.setRole(Admin::roleFromString(q.value(4).toString()));
        a.setActive(q.value(5).toInt() == 1);
        result.append(a);
    }
    return result;
}

Admin Database::adminByUsername(const QString& username) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT id,username,password,full_name,role,active FROM admins WHERE username=?");
    q.addBindValue(username);
    q.exec();
    if (q.next()) {
        Admin a;
        a.setId(q.value(0).toInt());
        a.setUsername(q.value(1).toString());
        a.setPasswordHash(q.value(2).toString());
        a.setFullName(q.value(3).toString());
        a.setRole(Admin::roleFromString(q.value(4).toString()));
        a.setActive(q.value(5).toInt() == 1);
        return a;
    }
    return Admin();
}

bool Database::adminExists(const QString& username) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM admins WHERE username=?");
    q.addBindValue(username);
    q.exec();
    return q.next() && q.value(0).toInt() > 0;
}

// ── ANALYTICS ──────────────────────────────────────────────────────────────

double Database::totalRevenue() const {
    // Sum grand totals = subtotal + tax - discount for all non-cancelled orders
    QSqlQuery q(m_db);
    q.exec("SELECT o.id, o.discount, o.tax_rate, SUM(oi.item_price * oi.quantity) as sub "
           "FROM orders o JOIN order_items oi ON o.id = oi.order_id "
           "WHERE o.status != 'Cancelled' GROUP BY o.id");
    double total = 0.0;
    while (q.next()) {
        double sub  = q.value(3).toDouble();
        double disc = q.value(1).toDouble();
        double tax  = q.value(2).toDouble();
        total += sub + sub*(tax/100.0) - sub*(disc/100.0);
    }
    return total;
}

double Database::revenueToday() const {
    // strftime works regardless of whether the stored timestamp uses
    // a space or 'T' separator (SQLite datetime() uses a space).
    QSqlQuery q(m_db);
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    q.prepare("SELECT o.discount, o.tax_rate, SUM(oi.item_price * oi.quantity) as sub "
              "FROM orders o JOIN order_items oi ON o.id = oi.order_id "
              "WHERE o.status != 'Cancelled' "
              "  AND strftime('%Y-%m-%d', o.created_at) = ? "
              "GROUP BY o.id");
    q.addBindValue(today);
    q.exec();
    double total = 0.0;
    while (q.next()) {
        double sub  = q.value(2).toDouble();
        double disc = q.value(0).toDouble();
        double tax  = q.value(1).toDouble();
        total += sub + sub*(tax/100.0) - sub*(disc/100.0);
    }
    return total;
}

int Database::totalOrderCount() const {
    QSqlQuery q("SELECT COUNT(*) FROM orders WHERE status != 'Cancelled'", m_db);
    return q.next() ? q.value(0).toInt() : 0;
}

int Database::orderCountToday() const {
    QSqlQuery q(m_db);
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    q.prepare("SELECT COUNT(*) FROM orders "
              "WHERE status != 'Cancelled' "
              "  AND strftime('%Y-%m-%d', created_at) = ?");
    q.addBindValue(today);
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

double Database::averageOrderValue() const {
    int cnt = totalOrderCount();
    return cnt > 0 ? totalRevenue() / cnt : 0.0;
}

QVector<QPair<QString,double>> Database::revenueByDay(int days) const {
    QVector<QPair<QString,double>> result;
    for (int i = days-1; i >= 0; --i) {
        QDate date = QDate::currentDate().addDays(-i);
        QString dateStr = date.toString("yyyy-MM-dd");
        QSqlQuery q(m_db);
        q.prepare("SELECT o.discount, o.tax_rate, SUM(oi.item_price * oi.quantity) "
                  "FROM orders o JOIN order_items oi ON o.id = oi.order_id "
                  "WHERE o.status != 'Cancelled' "
                  "  AND strftime('%Y-%m-%d', o.created_at) = ? "
                  "GROUP BY o.id");
        q.addBindValue(dateStr);
        q.exec();
        double dayTotal = 0.0;
        while (q.next()) {
            double sub  = q.value(2).toDouble();
            double disc = q.value(0).toDouble();
            double tax  = q.value(1).toDouble();
            dayTotal += sub + sub*(tax/100.0) - sub*(disc/100.0);
        }
        result.append({date.toString("MMM d"), dayTotal});
    }
    return result;
}

QVector<QPair<QString,int>> Database::topSellingItems(int limit) const {
    QVector<QPair<QString,int>> result;
    QSqlQuery q(m_db);
    q.prepare("SELECT item_name, SUM(quantity) as total FROM order_items "
              "JOIN orders ON orders.id = order_items.order_id "
              "WHERE orders.status != 'Cancelled' "
              "GROUP BY item_name ORDER BY total DESC LIMIT ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        result.append({q.value(0).toString(), q.value(1).toInt()});
    }
    return result;
}