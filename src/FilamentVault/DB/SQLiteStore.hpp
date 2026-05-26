#ifndef FILAMENTVAULT_SQLITE_STORE_HPP
#define FILAMENTVAULT_SQLITE_STORE_HPP

#include "SchemaManager.hpp"
#include "../Core/Spool.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace FilamentVault {

class SQLiteStore {
public:
    explicit SQLiteStore(const std::string &db_path);

    SQLite::Database &db() { return db_; }

    std::optional<int> insertSpool(const Spool &spool);
    bool               updateSpool(const Spool &spool);
    bool               archiveSpool(int id, bool archived = true);
    std::optional<Spool> getSpool(int id);
    std::vector<Spool>   listSpools(bool include_archived = false);
    std::vector<Spool>   filterByMaterial(const std::string &material);
    bool                 deductFilament(int spool_id, double amount_g);

private:
    SQLite::Database db_;

    Spool rowToSpool(SQLite::Statement &stmt);
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_SQLITE_STORE_HPP
