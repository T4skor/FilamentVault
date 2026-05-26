#ifndef FILAMENTVAULT_SPOOL_HPP
#define FILAMENTVAULT_SPOOL_HPP

#include <string>
#include <optional>

namespace FilamentVault {

struct Spool {
    int id = -1;
    std::string name;
    std::string brand;
    std::string material;
    std::string color_name;
    std::string color_hex;
    double diameter_mm = 1.75;
    double weight_total_g = 0.0;
    double weight_remain_g = 0.0;
    std::optional<double> density_g_cm3;
    int nozzle_temp_c          = 0; // other layers hotend, 0 = use default
    int nozzle_temp_initial_c  = 0; // first layer hotend,  0 = use default
    int bed_temp_c             = 0; // other layers bed,    0 = use default
    int bed_temp_initial_c     = 0; // first layer bed,     0 = use default
    std::string notes;
    bool archived = false;

    [[nodiscard]] double percentRemaining() const {
        if (weight_total_g <= 0.0) return 0.0;
        return (weight_remain_g / weight_total_g) * 100.0;
    }

    [[nodiscard]] bool canFulfill(double g) const {
        return weight_remain_g >= g;
    }
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_SPOOL_HPP
