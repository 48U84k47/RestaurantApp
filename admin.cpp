#include "admin.h"
#include <QCryptographicHash>

Admin::Admin()
    : m_id(-1), m_username(""), m_passwordHash(""), m_fullName(""),
    m_role(Role::Cashier), m_active(true)
{}

Admin::Admin(int id, const QString& username, const QString& passwordHash, Role role)
    : m_id(id), m_username(username), m_passwordHash(passwordHash),
    m_fullName(username), m_role(role), m_active(true)
{}

bool Admin::canManageMenu() const {
    return m_role == Role::SuperAdmin || m_role == Role::Manager;
}

bool Admin::canViewAnalytics() const {
    return m_role == Role::SuperAdmin || m_role == Role::Manager;
}

bool Admin::canManageUsers() const {
    return m_role == Role::SuperAdmin;
}

bool Admin::canProcessOrders() const {
    return true; // All roles can process orders
}

QString Admin::roleString() const {
    return roleToString(m_role);
}

QString Admin::displayName() const {
    return m_fullName.isEmpty() ? m_username : m_fullName;
}

QString Admin::hashPassword(const QString& password) {
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

bool Admin::verifyPassword(const QString& password, const QString& hash) {
    return hashPassword(password) == hash;
}

Admin::Role Admin::roleFromString(const QString& s) {
    if (s == "SuperAdmin") return Role::SuperAdmin;
    if (s == "Manager")    return Role::Manager;
    if (s == "Cashier")    return Role::Cashier;
    if (s == "Waiter")     return Role::Waiter;
    return Role::Cashier;
}

QString Admin::roleToString(Role r) {
    switch (r) {
    case Role::SuperAdmin: return "SuperAdmin";
    case Role::Manager:    return "Manager";
    case Role::Cashier:    return "Cashier";
    case Role::Waiter:     return "Waiter";
    default:               return "Cashier";
    }
}