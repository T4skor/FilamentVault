#ifndef FILAMENTVAULT_SPOOL_MATCHER_HPP
#define FILAMENTVAULT_SPOOL_MATCHER_HPP

#include "Spool.hpp"
#include <optional>
#include <string>

namespace FilamentVault {

class SQLiteStore;

class SpoolMatcher {
public:
    static double colorDistance(const std::string &hex_a, const std::string &hex_b);

    static std::optional<Spool> autoAssign(
        const std::string &material,
        const std::string &color_hex,
        double required_g,
        SQLiteStore &store);
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_SPOOL_MATCHER_HPP
