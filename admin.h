#ifndef ADMIN_H
#define ADMIN_H

#include <QString>

/**
 * @brief Represents an admin/staff user with elevated privileges.
 * Admins can manage menu items, view all orders, and access analytics.
 */
class Admin {
public:
    enum class Role {
        SuperAdmin,
        Manager,
        Cashier,
        Waiter
    };

    Admin();
    Admin(int id, const QString& username, const QString& passwordHash,
          Role role = Role::Manager);

    // Getters
    int     id()           const { return m_id; }
    QString username()     const { return m_username; }
    QString passwordHash() const { return m_passwordHash; }
    QString fullName()     const { return m_fullName; }
    Role    role()         const { return m_role; }
    bool    isActive()     const { return m_active; }

    // Setters
    void setId(int id)                      { m_id = id; }
    void setUsername(const QString& u)      { m_username = u; }
    void setPasswordHash(const QString& ph) { m_passwordHash = ph; }
    void setFullName(const QString& fn)     { m_fullName = fn; }
    void setRole(Role r)                    { m_role = r; }
    void setActive(bool active)             { m_active = active; }

    // Permission checks
    bool canManageMenu()     const;
    bool canViewAnalytics()  const;
    bool canManageUsers()    const;
    bool canProcessOrders()  const;

    // Utility
    QString roleString()  const;
    QString displayName() const;

    static QString hashPassword(const QString& password);
    static bool    verifyPassword(const QString& password, const QString& hash);
    static Role    roleFromString(const QString& s);
    static QString roleToString(Role r);

private:
    int     m_id;
    QString m_username;
    QString m_passwordHash;
    QString m_fullName;
    Role    m_role;
    bool    m_active;
};

#endif // ADMIN_H