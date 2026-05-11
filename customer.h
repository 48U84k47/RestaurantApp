#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <QString>
#include <QVector>

/**
 * @brief Represents a registered restaurant customer.
 * Stores personal info, credentials, and loyalty points.
 */
class Customer {
public:
    Customer();
    Customer(int id, const QString& name, const QString& email,
             const QString& phone, const QString& passwordHash);

    // Getters
    int     id()           const { return m_id; }
    QString name()         const { return m_name; }
    QString email()        const { return m_email; }
    QString phone()        const { return m_phone; }
    QString passwordHash() const { return m_passwordHash; }
    int     loyaltyPts()   const { return m_loyaltyPoints; }
    QString address()      const { return m_address; }
    bool    isActive()     const { return m_active; }

    // Setters
    void setId(int id)                       { m_id = id; }
    void setName(const QString& n)           { m_name = n; }
    void setEmail(const QString& e)          { m_email = e; }
    void setPhone(const QString& p)          { m_phone = p; }
    void setPasswordHash(const QString& ph)  { m_passwordHash = ph; }
    void setLoyaltyPoints(int pts)           { m_loyaltyPoints = pts; }
    void setAddress(const QString& a)        { m_address = a; }
    void setActive(bool active)              { m_active = active; }

    // Loyalty operations
    void addLoyaltyPoints(int pts);
    bool redeemPoints(int pts);

    // Utility
    QString displayName() const;

    // Simple hash utility (for demo — in prod use bcrypt/argon2)
    static QString hashPassword(const QString& password);
    static bool    verifyPassword(const QString& password, const QString& hash);

private:
    int     m_id;
    QString m_name;
    QString m_email;
    QString m_phone;
    QString m_passwordHash;
    int     m_loyaltyPoints;
    QString m_address;
    bool    m_active;
};

#endif // CUSTOMER_H