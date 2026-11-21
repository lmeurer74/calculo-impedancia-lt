#ifndef LINE_PARAMETERS_HPP
#define LINE_PARAMETERS_HPP

#include "LineGeometry.hpp"

namespace LineParameters {

struct CalculatedMatrices {
    // Matrizes primitivas completas (todos os subcondutores físicos e para-raios)
    ComplexMatrix Z_primitive; // [ohm/km]
    RealMatrix P_primitive;    // [km/F]
    
    // Matrizes reduzidas (feixes agrupados e para-raios aterrados eliminados por Kron)
    // Dimensão = número de fases (ex: 3 para circuito simples, 6 para circuito duplo)
    ComplexMatrix Z_phase;     // [ohm/km]
    RealMatrix P_phase;        // [km/F]
    RealMatrix C_phase;        // [F/km]
    ComplexMatrix Y_phase;     // [S/km]  (j * omega * C)
};

class LineParametersEngine {
public:
    // Monta as matrizes primitivas de Carson (Dubanton/Deri) e coeficientes de potencial de Maxwell
    static CalculatedMatrices compute(const LineSystemInput& input);

private:
    // Termo de correção de retorno pela terra de Carson via formulação fechada de profundidade complexa
    static Complex carsonCorrectionSelf(double h_avg, double radius_m, double freq, double rho);
    static Complex carsonCorrectionMutual(double h_i, double h_j, double d_ij, double D_ij, double freq, double rho);

    // Redução de Kron matricial: Z_red = Z_ww - Z_wg * Z_gg^-1 * Z_gw
    static ComplexMatrix applyKronReduction(const ComplexMatrix& Z, size_t numPhaseConds);
    static RealMatrix applyKronReduction(const RealMatrix& P, size_t numPhaseConds);

    // Redução e agrupamento de feixes de subcondutores para condutores de fase equivalentes
    static ComplexMatrix bundleReductionZ(const ComplexMatrix& Z_sub, const std::vector<PhysicalConductor>& phaseConds, size_t numPhases);
    static RealMatrix bundleReductionP(const RealMatrix& P_sub, const std::vector<PhysicalConductor>& phaseConds, size_t numPhases);
};

} // namespace LineParameters

#endif // LINE_PARAMETERS_HPP
