#include "VaultModel.hpp"
#include "../DB/SQLiteStore.hpp"

namespace FilamentVault {

VaultModel::~VaultModel() = default;

VaultModel::VaultModel(std::unique_ptr<SQLiteStore> store)
    : store_(std::move(store))
{
}

bool VaultModel::addSpool(const Spool &spool)
{
    auto id = store_->insertSpool(spool);
    if (id.has_value()) {
        if (inventoryCb_)
            inventoryCb_(id.value());
        return true;
    }
    return false;
}

bool VaultModel::updateSpool(const Spool &spool)
{
    if (store_->updateSpool(spool)) {
        if (inventoryCb_)
            inventoryCb_(spool.id);
        return true;
    }
    return false;
}

bool VaultModel::archiveSpool(int id, bool archived)
{
    if (store_->archiveSpool(id, archived)) {
        if (inventoryCb_)
            inventoryCb_(id);
        return true;
    }
    return false;
}

std::vector<Spool> VaultModel::listSpools(bool include_archived)
{
    return store_->listSpools(include_archived);
}

void VaultModel::onPrintStarted(const PrintJobInfo &job)
{
    if (selectionCb_)
        selectionCb_(job);
}

} // namespace FilamentVault
