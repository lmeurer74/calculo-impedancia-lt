#ifndef CONDUCTOR_MODEL_HPP
#define CONDUCTOR_MODEL_HPP

#include "InputData.hpp"

namespace LineParameters {

class ConductorModel {
public:
    // Converte a resistência CC a 20°C para a temperatura de operação T [°C]
    // R_dc(T) = R_dc(20) * [1 + alpha_20 * (T - 20)]
    static double correctResistanceTemperature(double r_dc_20, double T_op, double alpha20);

    // Calcula a resistência CA em ohm/km considerando efeito pelicular (NBR 11301 / Moore)
    static double calculateRac(double r_dc_temp, double outsideDiameterMm, double skinRatio, double freq);

    // Calcula a reatância interna X_int [ohm/km] a partir do GMR
    // X_int = omega * (mu_0 / 2pi) * ln(r / GMR)
    static double calculateInternalReactance(double outsideDiameterMm, double gmrMm, double freq);
};

} // namespace LineParameters

#endif // CONDUCTOR_MODEL_HPP
