/**
 * @file    main.cpp
 * @brief   Corrindor Restaurant Management System — entry point.
 *
 * Responsibilities
 * ────────────────
 * 1. Create the QApplication.
 * 2. Configure application-level metadata (name, version, org).
 * 3. Select a high-DPI-aware rendering policy (Qt 6 compatible).
 * 4. Load and apply the dark QSS theme from the Qt resource system.
 * 5. Initialise the SQLite database (creates tables + seeds demo data
 *    on first run; subsequent runs reuse the existing corrindor.db).
 * 6. Construct and show the MainWindow shell.
 * 7. Enter the Qt event loop.
 *
 * Default demo credentials (seeded on first run)
 * ───────────────────────────────────────────────
 *   Staff login   → username: admin    / password: admin123
 *   Staff login   → username: manager  / password: manager123
 *   Customer login→ email: customer@corrindor.com / password: customer123
 */

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

// Core
#include "database.h"

// Shell
#include "mainwindow.h"

// ── helpers ─────────────────────────────────────────────────────────────────

/**
 * Load the QSS stylesheet from the Qt resource file and apply it
 * globally to the QApplication instance.
 * Returns true on success, false if the resource could not be read.
 */
static bool applyStylesheet(QApplication& app) {
    QFile qssFile(":/styles/darktheme.qss");
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[Style] Could not open :/styles/darktheme.qss";
        return false;
    }
    app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    qssFile.close();
    return true;
}

/**
 * Initialise (or reopen) the SQLite database.
 * The database file is placed in the application's writable data
 * location so it survives between sessions and is not buried inside
 * the build directory.
 * Returns true when the database is ready; terminates the application
 * with an error dialog if it cannot be opened.
 */
static bool initDatabase(QApplication& app) {
    // Resolve a persistent, writable path for the DB file.
    // e.g. ~/.local/share/Corrindor/corrindor.db  (Linux / macOS)
    //      %APPDATA%\Corrindor\corrindor.db         (Windows)
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);   // create the directory if it does not exist
    QString dbPath = dataDir + "/corrindor.db";

    qInfo() << "[Database] Path:" << dbPath;

    Database& db = Database::instance();
    if (!db.initialize(dbPath)) {
        QMessageBox::critical(
            nullptr,
            "Database Error",
            "Corrindor could not open its database.\n\n"
            "Path: " + dbPath + "\n\n"
                           "Error: " + db.lastError() + "\n\n"
                                   "Please check that the application has write permission "
                                   "to its data directory and try again.");
        return false;
    }

    qInfo() << "[Database] Initialised successfully.";
    return true;
}

// ── entry point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // ── 1. High-DPI setup (must be done before QApplication) ──────────────
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5: opt-in to high-DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // ── 2. Create application ─────────────────────────────────────────────
    QApplication app(argc, argv);

    // ── 3. Application metadata ───────────────────────────────────────────
    app.setApplicationName("Corrindor Restaurant System");
    app.setApplicationDisplayName("Corrindor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Corrindor");
    app.setOrganizationDomain("corrindor.example.com");

    // ── 4. Default application font ───────────────────────────────────────
    // "Segoe UI" on Windows, "SF Pro Display" on macOS, system sans elsewhere.
    QFont appFont;
    appFont.setFamilies({"Segoe UI", "SF Pro Display", "Helvetica Neue", "Arial"});
    appFont.setPointSize(10);
    appFont.setWeight(QFont::Normal);
    app.setFont(appFont);

    // ── 5. Apply dark QSS theme ───────────────────────────────────────────
    if (!applyStylesheet(app)) {
        // Non-fatal: the app still functions without custom styling.
        qWarning() << "[Style] Running without custom stylesheet.";
    }

    // ── 6. Initialise database ────────────────────────────────────────────
    if (!initDatabase(app)) {
        return 1;   // fatal — error dialog already shown inside initDatabase()
    }

    // ── 7. Create and show the main window ────────────────────────────────
    MainWindow window;
    window.show();

    qInfo() << "[App] Corrindor started. Entering event loop.";

    // ── 8. Event loop ─────────────────────────────────────────────────────
    return app.exec();
}
