#ifndef FILAMENTVAULT_VAULT_MODEL_HPP
#define FILAMENTVAULT_VAULT_MODEL_HPP

#include "Spool.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace FilamentVault {

class SQLiteStore;

struct PrintJobInfo {
    std::string material;
    std::string color_hex;
    double required_filament_g = 0.0;
};

class VaultModel {
public:
    using InventoryCallback = std::function<void(int spoolId)>;
    using SelectionCallback = std::function<void(const PrintJobInfo &)>;

    explicit VaultModel(std::unique_ptr<SQLiteStore> store);
    ~VaultModel();

    void setInventoryCallback(InventoryCallback cb) { inventoryCb_ = std::move(cb); }
    void setSelectionCallback(SelectionCallback cb) { selectionCb_ = std::move(cb); }

    bool addSpool(const Spool &spool);
    bool updateSpool(const Spool &spool);
    bool archiveSpool(int id, bool archived = true);
    std::vector<Spool> listSpools(bool include_archived = false);
    void onPrintStarted(const PrintJobInfo &job);

private:
    std::unique_ptr<SQLiteStore> store_;
    InventoryCallback inventoryCb_;
    SelectionCallback selectionCb_;
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_VAULT_MODEL_HPP
