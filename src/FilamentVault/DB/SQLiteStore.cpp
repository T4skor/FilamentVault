#include "SQLiteStore.hpp"
#include <algorithm>

namespace FilamentVault {

SQLiteStore::SQLiteStore(const std::string &db_path)
    : db_(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    SchemaManager::migrate(db_);
}

Spool SQLiteStore::rowToSpool(SQLite::Statement &stmt)
{
    Spool s;
    s.id              = stmt.getColumn("id").getInt();
    s.name            = stmt.getColumn("name").getString();
    s.brand           = stmt.getColumn("brand").getString();
    s.material        = stmt.getColumn("material").getString();
    s.color_name      = stmt.getColumn("color_name").getString();
    s.color_hex       = stmt.getColumn("color_hex").getString();
    s.diameter_mm     = stmt.getColumn("diameter_mm").getDouble();
    s.weight_total_g  = stmt.getColumn("weight_total_g").getDouble();
    s.weight_remain_g = stmt.getColumn("weight_remain_g").getDouble();
    if (!stmt.isColumnNull("density_g_cm3"))
        s.density_g_cm3 = stmt.getColumn("density_g_cm3").getDouble();
    s.notes         = stmt.getColumn("notes").getString();
    s.archived      = stmt.getColumn("archived").getInt() != 0;
    s.nozzle_temp_c         = stmt.getColumn("nozzle_temp_c").getInt();
    s.nozzle_temp_initial_c = stmt.getColumn("nozzle_temp_initial_c").getInt();
    s.bed_temp_c            = stmt.getColumn("bed_temp_c").getInt();
    s.bed_temp_initial_c    = stmt.getColumn("bed_temp_initial_c").getInt();
    return s;
}

std::optional<int> SQLiteStore::insertSpool(const Spool &spool)
{
    try {
        SQLite::Statement stmt(db_,
            "INSERT INTO spools "
            "(name, brand, material, color_name, color_hex, diameter_mm, "
            " weight_total_g, weight_remain_g, density_g_cm3, "
            " nozzle_temp_c, nozzle_temp_initial_c, bed_temp_c, bed_temp_initial_c, "
            " notes, archived) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        stmt.bind(1, spool.name);
        stmt.bind(2, spool.brand);
        stmt.bind(3, spool.material);
        stmt.bind(4, spool.color_name);
        stmt.bind(5, spool.color_hex);
        stmt.bind(6, spool.diameter_mm);
        stmt.bind(7, spool.weight_total_g);
        stmt.bind(8, spool.weight_remain_g);
        if (spool.density_g_cm3.has_value())
            stmt.bind(9, spool.density_g_cm3.value());
        else
            stmt.bind(9);
        stmt.bind(10, spool.nozzle_temp_c);
        stmt.bind(11, spool.nozzle_temp_initial_c);
        stmt.bind(12, spool.bed_temp_c);
        stmt.bind(13, spool.bed_temp_initial_c);
        stmt.bind(14, spool.notes);
        stmt.bind(15, spool.archived ? 1 : 0);
        stmt.executeStep();
        return static_cast<int>(db_.getLastInsertRowid());
    } catch (...) {
        return std::nullopt;
    }
}

bool SQLiteStore::updateSpool(const Spool &spool)
{
    try {
        SQLite::Statement stmt(db_,
            "UPDATE spools SET "
            "name=?, brand=?, material=?, color_name=?, color_hex=?, "
            "diameter_mm=?, weight_total_g=?, weight_remain_g=?, "
            "density_g_cm3=?, nozzle_temp_c=?, nozzle_temp_initial_c=?, "
            "bed_temp_c=?, bed_temp_initial_c=?, "
            "notes=?, archived=?, "
            "updated_at=datetime('now') "
            "WHERE id=?");
        stmt.bind(1, spool.name);
        stmt.bind(2, spool.brand);
        stmt.bind(3, spool.material);
        stmt.bind(4, spool.color_name);
        stmt.bind(5, spool.color_hex);
        stmt.bind(6, spool.diameter_mm);
        stmt.bind(7, spool.weight_total_g);
        stmt.bind(8, spool.weight_remain_g);
        if (spool.density_g_cm3.has_value())
            stmt.bind(9, spool.density_g_cm3.value());
        else
            stmt.bind(9);
        stmt.bind(10, spool.nozzle_temp_c);
        stmt.bind(11, spool.nozzle_temp_initial_c);
        stmt.bind(12, spool.bed_temp_c);
        stmt.bind(13, spool.bed_temp_initial_c);
        stmt.bind(14, spool.notes);
        stmt.bind(15, spool.archived ? 1 : 0);
        stmt.bind(16, spool.id);
        stmt.executeStep();
        return db_.getTotalChanges() > 0;
    } catch (...) {
        return false;
    }
}

bool SQLiteStore::archiveSpool(int id, bool archived)
{
    try {
        SQLite::Statement stmt(db_,
            "UPDATE spools SET archived=?, updated_at=datetime('now') WHERE id=?");
        stmt.bind(1, archived ? 1 : 0);
        stmt.bind(2, id);
        stmt.executeStep();
        return db_.getTotalChanges() > 0;
    } catch (...) {
        return false;
    }
}

std::optional<Spool> SQLiteStore::getSpool(int id)
{
    try {
        SQLite::Statement stmt(db_, "SELECT * FROM spools WHERE id=?");
        stmt.bind(1, id);
        if (stmt.executeStep())
            return rowToSpool(stmt);
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<Spool> SQLiteStore::listSpools(bool include_archived)
{
    std::vector<Spool> result;
    try {
        std::string sql = "SELECT * FROM spools";
        if (!include_archived)
            sql += " WHERE archived=0";
        sql += " ORDER BY name";
        SQLite::Statement stmt(db_, sql);
        while (stmt.executeStep())
            result.push_back(rowToSpool(stmt));
    } catch (...) {
    }
    return result;
}

std::vector<Spool> SQLiteStore::filterByMaterial(const std::string &material)
{
    std::vector<Spool> result;
    try {
        SQLite::Statement stmt(db_,
            "SELECT * FROM spools WHERE material=? AND archived=0 ORDER BY name");
        stmt.bind(1, material);
        while (stmt.executeStep())
            result.push_back(rowToSpool(stmt));
    } catch (...) {
    }
    return result;
}

bool SQLiteStore::deductFilament(int spool_id, double amount_g)
{
    try {
        SQLite::Transaction tx(db_);

        SQLite::Statement sel(db_, "SELECT weight_remain_g FROM spools WHERE id=?");
        sel.bind(1, spool_id);
        if (!sel.executeStep())
            return false;

        double before = sel.getColumn(0).getDouble();
        double after  = std::max(0.0, before - amount_g);

        SQLite::Statement upd(db_,
            "UPDATE spools SET weight_remain_g=?, updated_at=datetime('now') WHERE id=?");
        upd.bind(1, after);
        upd.bind(2, spool_id);
        upd.executeStep();

        SQLite::Statement log(db_,
            "INSERT INTO consumption_log "
            "(spool_id, amount_g, remaining_before_g, remaining_after_g) "
            "VALUES (?, ?, ?, ?)");
        log.bind(1, spool_id);
        log.bind(2, amount_g);
        log.bind(3, before);
        log.bind(4, after);
        log.executeStep();

        tx.commit();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace FilamentVault
