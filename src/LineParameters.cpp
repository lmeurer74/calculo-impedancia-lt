#include "LineParameters.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace LineParameters {

// MU_0 e EPS_0 definidos em Types.hpp

Complex LineParametersEngine::carsonCorrectionSelf(double h_avg, double radius_m, double freq, double rho) {
    // Formulação de Profundidade Complexa de Retorno de Dubanton / Deri / Gary
    // p = sqrt(rho / (j * omega * mu_0)) [m]
    // Delta_Z = j*omega*(mu_0 / 2pi) * ln( (h + p) / h )
    double omega = 2.0 * PI * freq;
    Complex j(0.0, 1.0);
    Complex p = std::sqrt(Complex(0.0, -rho / (omega * MU_0)));

    Complex term = (j * omega * (MU_0 / (2.0 * PI))) * std::log((h_avg + p) / h_avg);
    return term * 1000.0; // Converte ohm/m para ohm/km
}

Complex LineParametersEngine::carsonCorrectionMutual(double h_i, double h_j, double d_ij, double D_ij, double freq, double rho) {
    double omega = 2.0 * PI * freq;
    Complex j(0.0, 1.0);
    Complex p = std::sqrt(Complex(0.0, -rho / (omega * MU_0)));

    double dx = std::sqrt(std::max(0.0, D_ij * D_ij - std::pow(h_i + h_j, 2.0)));
    Complex D_complex = std::sqrt(std::pow(h_i + h_j + 2.0 * p, 2.0) + std::pow(dx, 2.0));

    Complex term = (j * omega * (MU_0 / (2.0 * PI))) * std::log(D_complex / D_ij);
    return term * 1000.0; // Converte ohm/m para ohm/km
}

CalculatedMatrices LineParametersEngine::compute(const LineSystemInput& input) {
    CalculatedMatrices mats;
    std::vector<PhysicalConductor> allConds = LineGeometry::expandConductors(input);
    size_t totalConds = allConds.size();

    RealMatrix d = LineGeometry::buildDirectDistanceMatrix(allConds);
    RealMatrix D = LineGeometry::buildImageDistanceMatrix(allConds);

    double omega = 2.0 * PI * input.frequency;
    Complex j(0.0, 1.0);

    // 1. Montagem da Matriz Primitiva de Impedância [Z_primitive] (ohm/km)
    mats.Z_primitive = createComplexMatrix(totalConds, totalConds);
    for (size_t i = 0; i < totalConds; ++i) {
        double r_i_m = allConds[i].radiusMm / 1000.0;
        double h_i = allConds[i].h_avg;

        // Termo próprio: Z_ii = R_ac + X_int + j*omega*(mu0/2pi)*ln(2*hi / ri) + Delta_Z_earth
        double Z_ext_ind_m = (omega * MU_0 / (2.0 * PI)) * std::log(2.0 * h_i / r_i_m);
        Complex Z_self_classic = Complex(allConds[i].r_ac, allConds[i].x_int + Z_ext_ind_m * 1000.0);
        Complex delta_Z_earth = carsonCorrectionSelf(h_i, r_i_m, input.frequency, input.earthResistivity);

        mats.Z_primitive[i][i] = Z_self_classic + delta_Z_earth;

        for (size_t k = i + 1; k < totalConds; ++k) {
            double h_k = allConds[k].h_avg;
            double d_ik = d[i][k];
            double D_ik = D[i][k];

            double Z_mut_ind_m = (omega * MU_0 / (2.0 * PI)) * std::log(D_ik / d_ik);
            Complex Z_mut_classic = Complex(0.0, Z_mut_ind_m * 1000.0);
            Complex delta_Z_mut_earth = carsonCorrectionMutual(h_i, h_k, d_ik, D_ik, input.frequency, input.earthResistivity);

            Complex Z_mut = Z_mut_classic + delta_Z_mut_earth;
            mats.Z_primitive[i][k] = Z_mut;
            mats.Z_primitive[k][i] = Z_mut;
        }
    }

    // 2. Montagem da Matriz Primitiva de Potenciais de Maxwell [P_primitive] (km/F)
    mats.P_primitive = createRealMatrix(totalConds, totalConds);
    double P_factor = 1.0 / (2.0 * PI * EPS_0); // m/F ~ 17.975e9

    for (size_t i = 0; i < totalConds; ++i) {
        double r_i_m = allConds[i].radiusMm / 1000.0;
        double h_i = allConds[i].h_avg;
        mats.P_primitive[i][i] = (P_factor * std::log(2.0 * h_i / r_i_m)) / 1000.0;

        for (size_t k = i + 1; k < totalConds; ++k) {
            double d_ik = d[i][k];
            double D_ik = D[i][k];
            double p_val = (P_factor * std::log(D_ik / d_ik)) / 1000.0;
            mats.P_primitive[i][k] = p_val;
            mats.P_primitive[k][i] = p_val;
        }
    }

    // Contagem de condutores de fase e de para-raios
    size_t numPhaseSubconds = 0;
    for (const auto& c : allConds) {
        if (!c.isGroundWire) numPhaseSubconds++;
    }
    size_t numGW = totalConds - numPhaseSubconds;

    // 3. Eliminação dos Para-Raios via Redução de Kron
    ComplexMatrix Z_after_gw = mats.Z_primitive;
    RealMatrix P_after_gw = mats.P_primitive;

    if (numGW > 0) {
        Z_after_gw = applyKronReduction(mats.Z_primitive, numPhaseSubconds);
        P_after_gw = applyKronReduction(mats.P_primitive, numPhaseSubconds);
    }

    // 4. Agrupamento de Subcondutores do Feixe (Redução de Feixe)
    std::vector<PhysicalConductor> phaseSubconds(allConds.begin(), allConds.begin() + numPhaseSubconds);
    size_t numPhases = input.phases.size();

    mats.Z_phase = bundleReductionZ(Z_after_gw, phaseSubconds, numPhases);
    mats.P_phase = bundleReductionP(P_after_gw, phaseSubconds, numPhases);

    // 5. Inversão de [P_phase] para obtenção da Matriz de Capacitâncias [C_phase] (F/km)
    invertMatrix(mats.P_phase, mats.C_phase);

    // 6. Matriz de Admitância Shunt [Y_phase] = j * omega * [C_phase]
    mats.Y_phase = createComplexMatrix(numPhases, numPhases);
    for (size_t i = 0; i < numPhases; ++i) {
        for (size_t k = 0; k < numPhases; ++k) {
            mats.Y_phase[i][k] = Complex(0.0, omega * mats.C_phase[i][k]);
        }
    }

    return mats;
}

ComplexMatrix LineParametersEngine::applyKronReduction(const ComplexMatrix& Z, size_t numPhaseConds) {
    size_t total = Z.size();
    size_t numGW = total - numPhaseConds;
    if (numGW == 0) return Z;

    ComplexMatrix Z_ww = createComplexMatrix(numPhaseConds, numPhaseConds);
    ComplexMatrix Z_wg = createComplexMatrix(numPhaseConds, numGW);
    ComplexMatrix Z_gw = createComplexMatrix(numGW, numPhaseConds);
    ComplexMatrix Z_gg = createComplexMatrix(numGW, numGW);

    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) Z_ww[i][j] = Z[i][j];
        for (size_t j = 0; j < numGW; ++j) Z_wg[i][j] = Z[i][numPhaseConds + j];
    }
    for (size_t i = 0; i < numGW; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) Z_gw[i][j] = Z[numPhaseConds + i][j];
        for (size_t j = 0; j < numGW; ++j) Z_gg[i][j] = Z[numPhaseConds + i][numPhaseConds + j];
    }

    ComplexMatrix Z_gg_inv;
    if (!invertMatrix(Z_gg, Z_gg_inv)) {
        std::cerr << "Erro: Nao foi possivel inverter Z_gg na Reducao de Kron!" << std::endl;
        return Z_ww;
    }

    ComplexMatrix temp = createComplexMatrix(numPhaseConds, numGW);
    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numGW; ++j) {
            for (size_t k = 0; k < numGW; ++k) {
                temp[i][j] += Z_wg[i][k] * Z_gg_inv[k][j];
            }
        }
    }

    ComplexMatrix Z_red = Z_ww;
    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) {
            for (size_t k = 0; k < numGW; ++k) {
                Z_red[i][j] -= temp[i][k] * Z_gw[k][j];
            }
        }
    }

    return Z_red;
}

RealMatrix LineParametersEngine::applyKronReduction(const RealMatrix& P, size_t numPhaseConds) {
    size_t total = P.size();
    size_t numGW = total - numPhaseConds;
    if (numGW == 0) return P;

    RealMatrix P_ww = createRealMatrix(numPhaseConds, numPhaseConds);
    RealMatrix P_wg = createRealMatrix(numPhaseConds, numGW);
    RealMatrix P_gw = createRealMatrix(numGW, numPhaseConds);
    RealMatrix P_gg = createRealMatrix(numGW, numGW);

    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) P_ww[i][j] = P[i][j];
        for (size_t j = 0; j < numGW; ++j) P_wg[i][j] = P[i][numPhaseConds + j];
    }
    for (size_t i = 0; i < numGW; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) P_gw[i][j] = P[numPhaseConds + i][j];
        for (size_t j = 0; j < numGW; ++j) P_gg[i][j] = P[numPhaseConds + i][numPhaseConds + j];
    }

    RealMatrix P_gg_inv;
    if (!invertMatrix(P_gg, P_gg_inv)) {
        std::cerr << "Erro: Nao foi possivel inverter P_gg na Reducao de Kron!" << std::endl;
        return P_ww;
    }

    RealMatrix temp = createRealMatrix(numPhaseConds, numGW);
    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numGW; ++j) {
            for (size_t k = 0; k < numGW; ++k) {
                temp[i][j] += P_wg[i][k] * P_gg_inv[k][j];
            }
        }
    }

    RealMatrix P_red = P_ww;
    for (size_t i = 0; i < numPhaseConds; ++i) {
        for (size_t j = 0; j < numPhaseConds; ++j) {
            for (size_t k = 0; k < numGW; ++k) {
                P_red[i][j] -= temp[i][k] * P_gw[k][j];
            }
        }
    }

    return P_red;
}

ComplexMatrix LineParametersEngine::bundleReductionZ(const ComplexMatrix& Z_sub,
                                                     const std::vector<PhysicalConductor>& phaseConds,
                                                     size_t numPhases) {
    std::vector<std::vector<size_t>> phaseIndices(numPhases);
    for (size_t i = 0; i < phaseConds.size(); ++i) {
        int p = phaseConds[i].phaseIndex;
        if (p >= 0 && static_cast<size_t>(p) < numPhases) {
            phaseIndices[p].push_back(i);
        }
    }

    ComplexMatrix Z_phase = createComplexMatrix(numPhases, numPhases);
    for (size_t p = 0; p < numPhases; ++p) {
        size_t Np = phaseIndices[p].size();
        for (size_t q = 0; q < numPhases; ++q) {
            size_t Nq = phaseIndices[q].size();
            Complex sum(0.0, 0.0);
            for (size_t i : phaseIndices[p]) {
                for (size_t j : phaseIndices[q]) {
                    sum += Z_sub[i][j];
                }
            }
            Z_phase[p][q] = sum / static_cast<double>(Np * Nq);
        }
    }
    return Z_phase;
}

RealMatrix LineParametersEngine::bundleReductionP(const RealMatrix& P_sub,
                                                 const std::vector<PhysicalConductor>& phaseConds,
                                                 size_t numPhases) {
    std::vector<std::vector<size_t>> phaseIndices(numPhases);
    for (size_t i = 0; i < phaseConds.size(); ++i) {
        int p = phaseConds[i].phaseIndex;
        if (p >= 0 && static_cast<size_t>(p) < numPhases) {
            phaseIndices[p].push_back(i);
        }
    }

    RealMatrix P_phase = createRealMatrix(numPhases, numPhases);
    for (size_t p = 0; p < numPhases; ++p) {
        size_t Np = phaseIndices[p].size();
        for (size_t q = 0; q < numPhases; ++q) {
            size_t Nq = phaseIndices[q].size();
            double sum = 0.0;
            for (size_t i : phaseIndices[p]) {
                for (size_t j : phaseIndices[q]) {
                    sum += P_sub[i][j];
                }
            }
            P_phase[p][q] = sum / static_cast<double>(Np * Nq);
        }
    }
    return P_phase;
}

} // namespace LineParameters
