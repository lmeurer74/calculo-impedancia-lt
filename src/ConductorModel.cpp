#include "ConductorModel.hpp"
#include <cmath>
#include <algorithm>

namespace LineParameters {

// MU_0 definido em Types.hpp

double ConductorModel::correctResistanceTemperature(double r_dc_20, double T_op, double alpha20) {
    return r_dc_20 * (1.0 + alpha20 * (T_op - 20.0));
}

double ConductorModel::calculateRac(double r_dc_temp, double outsideDiameterMm, double skinRatio, double freq) {
    // Efeito pelicular conforme IEC / NBR 11301 para condutores cilíndricos/tubulares
    double r_dc_m = r_dc_temp / 1000.0; // ohm/m
    if (r_dc_m <= 0.0) return 0.0;

    double ks = 1.0;
    if (skinRatio < 0.49) {
        ks = std::max(0.5, 2.0 * skinRatio);
    }

    double xs2 = (8.0 * PI * freq * ks * 1e-7) / r_dc_m;
    double xs = std::sqrt(xs2);

    double ys = 0.0;
    if (xs <= 2.8) {
        double xs4 = xs2 * xs2;
        ys = xs4 / (192.0 + 0.8 * xs4);
    } else {
        ys = (xs / (2.0 * std::sqrt(2.0))) - 0.26;
    }

    return r_dc_temp * (1.0 + ys);
}

double ConductorModel::calculateInternalReactance(double outsideDiameterMm, double gmrMm, double freq) {
    double radiusMm = outsideDiameterMm / 2.0;
    if (gmrMm <= 0.0 || radiusMm <= gmrMm) {
        return 0.0;
    }
    double omega = 2.0 * PI * freq;
    double L_int_per_m = (MU_0 / (2.0 * PI)) * std::log(radiusMm / gmrMm);
    return omega * L_int_per_m * 1000.0; // ohm/km
}

} // namespace LineParameters
