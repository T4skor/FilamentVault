#ifndef FILAMENTVAULT_SCHEMA_MANAGER_HPP
#define FILAMENTVAULT_SCHEMA_MANAGER_HPP

#include <SQLiteCpp/SQLiteCpp.h>

namespace FilamentVault {

class SchemaManager {
public:
    static void migrate(SQLite::Database &db);

private:
    static constexpr int CURRENT_VERSION = 4;

    static void migrateToV1(SQLite::Database &db);
    static void migrateToV2(SQLite::Database &db);
    static void migrateToV3(SQLite::Database &db);
    static void migrateToV4(SQLite::Database &db);
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_SCHEMA_MANAGER_HPP
