#include "LineGeometry.hpp"
#include "ConductorModel.hpp"
#include <cmath>
#include <algorithm>

namespace LineParameters {

double LineGeometry::calculateMechanicalSag(double weightPerMeter, double diameterMm,
                                            const EnvironmentalData& env) {
    if (env.horizontalTension <= 0.0 || env.spanLength <= 0.0) return 0.0;

    // Carga de gelo: w_i = pi * t * (d + t) * densidade_gelo (kg/m)
    double wi = 0.0;
    if (env.iceThickness > 0.0) {
        double d_m = diameterMm / 1000.0;
        double t_m = env.iceThickness / 1000.0;
        double iceDensity = 915.0; // kg/m³
        wi = PI * t_m * (d_m + t_m) * iceDensity;
    }

    // Carga de vento horizontal: w_w = P_vento * (d + 2*t) [kg/m]
    double ww = 0.0;
    if (env.windPressure > 0.0) {
        double total_d_m = (diameterMm + 2.0 * env.iceThickness) / 1000.0;
        ww = env.windPressure * total_d_m;
    }

    // Carga resultante total: w_r = sqrt((w_c + w_i)^2 + w_w^2)
    double wr = std::sqrt(std::pow(weightPerMeter + wi, 2.0) + std::pow(ww, 2.0));

    // Flecha vertical projetada na direção da gravidade:
    // d_flecha = ((w_c + w_i) * L²) / (8 * T)
    double sag_vert = ((weightPerMeter + wi) * std::pow(env.spanLength, 2.0)) / (8.0 * env.horizontalTension);
    return sag_vert;
}

std::vector<PhysicalConductor> LineGeometry::expandConductors(const LineSystemInput& input) {
    std::vector<PhysicalConductor> result;

    // 1. Processa fases de potência
    for (size_t p = 0; p < input.phases.size(); ++p) {
        const auto& phase = input.phases[p];
        const auto& cond = phase.conductor;
        const auto& b = phase.bundle;

        // Determinação da flecha e altura no meio do vão
        double y_mid = phase.y_tower;
        if (phase.hasMidSpanHeight) {
            if (phase.y_midSpan > 0.0) {
                y_mid = phase.y_midSpan;
            } else if (phase.sagProject > 0.0) {
                y_mid = phase.y_tower - phase.sagProject;
            }
        } else {
            // Se o usuário não forneceu dados de projeto, calcula a flecha mecânica
            double sag = calculateMechanicalSag(cond.weightPerMeter, cond.outsideDiameter, input.environment);
            y_mid = phase.y_tower - sag;
        }

        // Altura média equivalente recomendada no EMTP e didáticos:
        // h_avg = (2 * y_mid + y_tower) / 3
        double h_avg_phase = (2.0 * y_mid + phase.y_tower) / 3.0;

        // Propriedades elétricas individuais
        double r_temp = ConductorModel::correctResistanceTemperature(cond.r_dc_20,
                                                                     input.environment.operatingTemperature,
                                                                     cond.alpha20);
        double r_ac = ConductorModel::calculateRac(r_temp, cond.outsideDiameter, cond.skinRatio, input.frequency);
        double x_int = ConductorModel::calculateInternalReactance(cond.outsideDiameter, cond.gmr, input.frequency);
        double radiusMm = cond.outsideDiameter / 2.0;

        int N = std::max(1, b.numSubconductors);
        double alphaRad = b.angleDeg * PI / 180.0;

        if (N == 1 || b.arrangement == BundleArrangement::Single) {
            PhysicalConductor pc;
            pc.id = phase.phaseId;
            pc.phaseIndex = static_cast<int>(p);
            pc.circuitId = phase.circuitId;
            pc.isGroundWire = false;
            pc.x = phase.x_tower;
            pc.y_tower = phase.y_tower;
            pc.y_mid = y_mid;
            pc.h_avg = h_avg_phase;
            pc.radiusMm = radiusMm;
            pc.gmrMm = cond.gmr;
            pc.r_ac = r_ac;
            pc.x_int = x_int;
            result.push_back(pc);
        } else if (N == 2 || b.arrangement == BundleArrangement::Horizontal) {
            // Feixe duplo (lado a lado inclinado por alpha)
            double dX = (b.spacing / 2.0) * std::cos(alphaRad);
            double dY = (b.spacing / 2.0) * std::sin(alphaRad);

            for (int k = 0; k < 2; ++k) {
                double sign = (k == 0) ? -1.0 : 1.0;
                PhysicalConductor pc;
                pc.id = phase.phaseId + "_s" + std::to_string(k + 1);
                pc.phaseIndex = static_cast<int>(p);
                pc.circuitId = phase.circuitId;
                pc.isGroundWire = false;
                pc.x = phase.x_tower + sign * dX;
                pc.y_tower = phase.y_tower + sign * dY;
                pc.y_mid = y_mid + sign * dY;
                pc.h_avg = h_avg_phase + sign * dY;
                pc.radiusMm = radiusMm;
                pc.gmrMm = cond.gmr;
                pc.r_ac = r_ac;
                pc.x_int = x_int;
                result.push_back(pc);
            }
        } else if (N == 3 || b.arrangement == BundleArrangement::Triangular) {
            // Triângulo equilátero circunscrito com raio A = spacing / sqrt(3)
            double A = b.spacing / std::sqrt(3.0);
            for (int k = 0; k < 3; ++k) {
                double theta = alphaRad + (2.0 * PI * k) / 3.0;
                double dX = A * std::cos(theta);
                double dY = A * std::sin(theta);

                PhysicalConductor pc;
                pc.id = phase.phaseId + "_s" + std::to_string(k + 1);
                pc.phaseIndex = static_cast<int>(p);
                pc.circuitId = phase.circuitId;
                pc.isGroundWire = false;
                pc.x = phase.x_tower + dX;
                pc.y_tower = phase.y_tower + dY;
                pc.y_mid = y_mid + dY;
                pc.h_avg = h_avg_phase + dY;
                pc.radiusMm = radiusMm;
                pc.gmrMm = cond.gmr;
                pc.r_ac = r_ac;
                pc.x_int = x_int;
                result.push_back(pc);
            }
        } else if (N == 4 && b.arrangement == BundleArrangement::Square) {
            // Quadrado com raio circunscrito A = spacing / sqrt(2)
            double A = b.spacing / std::sqrt(2.0);
            for (int k = 0; k < 4; ++k) {
                double theta = alphaRad + (PI / 4.0) + (k * PI / 2.0);
                double dX = A * std::cos(theta);
                double dY = A * std::sin(theta);

                PhysicalConductor pc;
                pc.id = phase.phaseId + "_s" + std::to_string(k + 1);
                pc.phaseIndex = static_cast<int>(p);
                pc.circuitId = phase.circuitId;
                pc.isGroundWire = false;
                pc.x = phase.x_tower + dX;
                pc.y_tower = phase.y_tower + dY;
                pc.y_mid = y_mid + dY;
                pc.h_avg = h_avg_phase + dY;
                pc.radiusMm = radiusMm;
                pc.gmrMm = cond.gmr;
                pc.r_ac = r_ac;
                pc.x_int = x_int;
                result.push_back(pc);
            }
        } else {
            // N subcondutores uniformemente distribuídos em círculo de raio A = spacing / (2 * sin(pi/N))
            double A = b.spacing / (2.0 * std::sin(PI / N));
            for (int k = 0; k < N; ++k) {
                double theta = alphaRad + (2.0 * PI * k) / N;
                double dX = A * std::cos(theta);
                double dY = A * std::sin(theta);

                PhysicalConductor pc;
                pc.id = phase.phaseId + "_s" + std::to_string(k + 1);
                pc.phaseIndex = static_cast<int>(p);
                pc.circuitId = phase.circuitId;
                pc.isGroundWire = false;
                pc.x = phase.x_tower + dX;
                pc.y_tower = phase.y_tower + dY;
                pc.y_mid = y_mid + dY;
                pc.h_avg = h_avg_phase + dY;
                pc.radiusMm = radiusMm;
                pc.gmrMm = cond.gmr;
                pc.r_ac = r_ac;
                pc.x_int = x_int;
                result.push_back(pc);
            }
        }
    }

    // 2. Processa cabos para-raios (ground wires)
    for (size_t g = 0; g < input.groundWires.size(); ++g) {
        const auto& gw = input.groundWires[g];
        const auto& cond = gw.conductor;

        double y_mid = gw.y_tower;
        if (gw.hasMidSpanHeight) {
            if (gw.y_midSpan > 0.0) {
                y_mid = gw.y_midSpan;
            } else if (gw.sagProject > 0.0) {
                y_mid = gw.y_tower - gw.sagProject;
            }
        } else {
            double sag = calculateMechanicalSag(cond.weightPerMeter, cond.outsideDiameter, input.environment);
            y_mid = gw.y_tower - sag;
        }

        double h_avg_gw = (2.0 * y_mid + gw.y_tower) / 3.0;

        double r_temp = ConductorModel::correctResistanceTemperature(cond.r_dc_20,
                                                                     input.environment.ambientTemperature,
                                                                     cond.alpha20);
        double r_ac = ConductorModel::calculateRac(r_temp, cond.outsideDiameter, cond.skinRatio, input.frequency);
        double x_int = ConductorModel::calculateInternalReactance(cond.outsideDiameter, cond.gmr, input.frequency);
        double radiusMm = cond.outsideDiameter / 2.0;

        PhysicalConductor pc;
        pc.id = gw.wireId;
        pc.phaseIndex = -1; // Para-raios
        pc.circuitId = 0;
        pc.isGroundWire = true;
        pc.grounding = gw.grounding;
        pc.x = gw.x_tower;
        pc.y_tower = gw.y_tower;
        pc.y_mid = y_mid;
        pc.h_avg = h_avg_gw;
        pc.radiusMm = radiusMm;
        pc.gmrMm = cond.gmr;
        pc.r_ac = r_ac;
        pc.x_int = x_int;
        result.push_back(pc);
    }

    return result;
}

RealMatrix LineGeometry::buildDirectDistanceMatrix(const std::vector<PhysicalConductor>& conds) {
    size_t n = conds.size();
    RealMatrix d = createRealMatrix(n, n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (i == j) {
                d[i][j] = 0.0;
            } else {
                double dx = conds[i].x - conds[j].x;
                double dy = conds[i].h_avg - conds[j].h_avg;
                d[i][j] = std::sqrt(dx * dx + dy * dy);
            }
        }
    }
    return d;
}

RealMatrix LineGeometry::buildImageDistanceMatrix(const std::vector<PhysicalConductor>& conds) {
    size_t n = conds.size();
    RealMatrix D = createRealMatrix(n, n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            double dx = conds[i].x - conds[j].x;
            double dy = conds[i].h_avg + conds[j].h_avg; // Distância ao espelho (-h_j)
            D[i][j] = std::sqrt(dx * dx + dy * dy);
        }
    }
    return D;
}

} // namespace LineParameters
