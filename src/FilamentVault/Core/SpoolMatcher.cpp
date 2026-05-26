#include "SpoolMatcher.hpp"
#include "../DB/SQLiteStore.hpp"
#include <cmath>
#include <limits>

namespace FilamentVault {

static int hexCharToInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

static int hexPairToByte(const std::string &hex, size_t start)
{
    return (hexCharToInt(hex[start]) << 4) | hexCharToInt(hex[start + 1]);
}

double SpoolMatcher::colorDistance(const std::string &hex_a, const std::string &hex_b)
{
    std::string a = hex_a;
    std::string b = hex_b;

    if (!a.empty() && a[0] == '#') a = a.substr(1);
    if (!b.empty() && b[0] == '#') b = b.substr(1);

    if (a.length() < 6) a = "000000";
    if (b.length() < 6) b = "000000";

    int r1 = hexPairToByte(a, 0);
    int g1 = hexPairToByte(a, 2);
    int b1 = hexPairToByte(a, 4);
    int r2 = hexPairToByte(b, 0);
    int g2 = hexPairToByte(b, 2);
    int b2 = hexPairToByte(b, 4);

    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;

    return std::sqrt(static_cast<double>(dr * dr + dg * dg + db * db));
}

std::optional<Spool> SpoolMatcher::autoAssign(
    const std::string &material,
    const std::string &color_hex,
    double required_g,
    SQLiteStore &store)
{
    auto candidates = store.filterByMaterial(material);

    std::optional<Spool> exact_match;
    double exact_distance = std::numeric_limits<double>::max();
    std::optional<Spool> fallback;

    for (const auto &spool : candidates) {
        if (spool.archived || !spool.canFulfill(required_g))
            continue;

        double dist = colorDistance(spool.color_hex, color_hex);

        if (dist < exact_distance) {
            exact_match = spool;
            exact_distance = dist;
        }

        if (!fallback.has_value()) {
            fallback = spool;
        }
    }

    if (exact_match.has_value() && exact_distance < 50.0)
        return exact_match;

    if (fallback.has_value())
        return fallback;

    return std::nullopt;
}

} // namespace FilamentVault
