#include "customer.h"
#include <QCryptographicHash>

Customer::Customer()
    : m_id(-1), m_name(""), m_email(""), m_phone(""),
    m_passwordHash(""), m_loyaltyPoints(0), m_address(""), m_active(true)
{}

Customer::Customer(int id, const QString& name, const QString& email,
                   const QString& phone, const QString& passwordHash)
    : m_id(id), m_name(name), m_email(email), m_phone(phone),
    m_passwordHash(passwordHash), m_loyaltyPoints(0), m_address(""), m_active(true)
{}

void Customer::addLoyaltyPoints(int pts) {
    m_loyaltyPoints += pts;
}

bool Customer::redeemPoints(int pts) {
    if (m_loyaltyPoints >= pts) {
        m_loyaltyPoints -= pts;
        return true;
    }
    return false;
}

QString Customer::displayName() const {
    return m_name.isEmpty() ? m_email : m_name;
}

QString Customer::hashPassword(const QString& password) {
    // SHA-256 hash for demo purposes
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

bool Customer::verifyPassword(const QString& password, const QString& hash) {
    return hashPassword(password) == hash;
}