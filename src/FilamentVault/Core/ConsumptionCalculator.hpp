#ifndef FILAMENTVAULT_CONSUMPTION_CALCULATOR_HPP
#define FILAMENTVAULT_CONSUMPTION_CALCULATOR_HPP

namespace FilamentVault {

struct ConsumptionCalculator {
    static double fromVolume(double volume_mm3, double density_g_cm3) {
        return volume_mm3 * density_g_cm3 / 1000.0;
    }

    static double fromLength(double length_mm, double diameter_mm, double density_g_cm3) {
        double radius = diameter_mm / 2.0;
        double area = 3.14159265358979323846 * radius * radius;
        double volume = area * length_mm;
        return fromVolume(volume, density_g_cm3);
    }

    static double withWasteFactor(double grams, double factor = 1.05) {
        return grams * factor;
    }
};

} // namespace FilamentVault

#endif // FILAMENTVAULT_CONSUMPTION_CALCULATOR_HPP
