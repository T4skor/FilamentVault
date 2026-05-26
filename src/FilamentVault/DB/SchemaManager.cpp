#include "SchemaManager.hpp"

namespace FilamentVault {

void SchemaManager::migrate(SQLite::Database &db)
{
    bool has_version_table = false;
    try {
        SQLite::Statement chk(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='schema_version'");
        has_version_table = chk.executeStep();
    } catch (...) {
    }

    int version = 0;
    if (has_version_table) {
        try {
            SQLite::Statement stmt(db, "SELECT COALESCE(MAX(version), 0) FROM schema_version");
            if (stmt.executeStep())
                version = stmt.getColumn(0).getInt();
        } catch (...) {
        }
    }

    if (version < 1) {
        migrateToV1(db);
        version = 1;
    }
    if (version < 2) {
        migrateToV2(db);
        version = 2;
    }
    if (version < 3) {
        migrateToV3(db);
        version = 3;
    }
    if (version < 4) {
        migrateToV4(db);
        version = 4;
    }
}

void SchemaManager::migrateToV1(SQLite::Database &db)
{
    db.exec("CREATE TABLE IF NOT EXISTS schema_version ("
            "version INTEGER NOT NULL,"
            "applied_at TEXT NOT NULL DEFAULT (datetime('now')))");

    db.exec("CREATE TABLE IF NOT EXISTS spools ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "brand TEXT NOT NULL DEFAULT '',"
            "material TEXT NOT NULL,"
            "color_name TEXT NOT NULL DEFAULT '',"
            "color_hex TEXT NOT NULL DEFAULT '#FFFFFF',"
            "diameter_mm REAL NOT NULL DEFAULT 1.75,"
            "weight_total_g REAL NOT NULL,"
            "weight_remain_g REAL NOT NULL,"
            "density_g_cm3 REAL,"
            "notes TEXT NOT NULL DEFAULT '',"
            "archived INTEGER NOT NULL DEFAULT 0,"
            "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
            "updated_at TEXT NOT NULL DEFAULT (datetime('now')))");

    db.exec("CREATE TABLE IF NOT EXISTS consumption_log ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "spool_id INTEGER NOT NULL,"
            "amount_g REAL NOT NULL,"
            "remaining_before_g REAL NOT NULL,"
            "remaining_after_g REAL NOT NULL,"
            "consumed_at TEXT NOT NULL DEFAULT (datetime('now')),"
            "FOREIGN KEY (spool_id) REFERENCES spools(id) ON DELETE CASCADE)");

    db.exec("CREATE TABLE IF NOT EXISTS assignment_rules ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "material TEXT NOT NULL,"
            "color_hex TEXT NOT NULL DEFAULT '',"
            "spool_id INTEGER,"
            "priority INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY (spool_id) REFERENCES spools(id) ON DELETE SET NULL)");

    db.exec("CREATE INDEX IF NOT EXISTS idx_spools_material ON spools(material)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_spools_archived ON spools(archived)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_consumption_log_spool ON consumption_log(spool_id)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_assignment_rules_material ON assignment_rules(material)");

    SQLite::Statement stmt(db, "INSERT INTO schema_version (version) VALUES (1)");
    stmt.executeStep();
}

void SchemaManager::migrateToV2(SQLite::Database &db)
{
    db.exec("CREATE INDEX IF NOT EXISTS idx_spools_color_hex ON spools(color_hex)");

    SQLite::Statement stmt(db, "INSERT INTO schema_version (version) VALUES (2)");
    stmt.executeStep();
}

void SchemaManager::migrateToV3(SQLite::Database &db)
{
    db.exec("ALTER TABLE spools ADD COLUMN nozzle_temp_c INTEGER NOT NULL DEFAULT 0");
    db.exec("ALTER TABLE spools ADD COLUMN bed_temp_c INTEGER NOT NULL DEFAULT 0");

    SQLite::Statement stmt(db, "INSERT INTO schema_version (version) VALUES (3)");
    stmt.executeStep();
}

void SchemaManager::migrateToV4(SQLite::Database &db)
{
    db.exec("ALTER TABLE spools ADD COLUMN nozzle_temp_initial_c INTEGER NOT NULL DEFAULT 0");
    db.exec("ALTER TABLE spools ADD COLUMN bed_temp_initial_c INTEGER NOT NULL DEFAULT 0");

    SQLite::Statement stmt(db, "INSERT INTO schema_version (version) VALUES (4)");
    stmt.executeStep();
}

} // namespace FilamentVault
