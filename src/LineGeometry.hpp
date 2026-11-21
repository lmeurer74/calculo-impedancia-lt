#ifndef LINE_GEOMETRY_HPP
#define LINE_GEOMETRY_HPP

#include "InputData.hpp"
#include <vector>

namespace LineParameters {

struct PhysicalConductor {
    std::string id;          // Ex: "A_s1", "GW1"
    int phaseIndex = -1;     // Índice da fase associada (0, 1, 2, ... ou -1 para para-raios)
    int circuitId = 1;       // Circuito 1 ou 2
    bool isGroundWire = false;
    GroundWireGrounding grounding = GroundWireGrounding::Continuous;

    double x = 0.0;          // Coordenada horizontal [m]
    double y_tower = 0.0;    // Cota na torre [m]
    double y_mid = 0.0;      // Cota no meio do vão [m]
    double h_avg = 0.0;      // Altura média equivalente h_avg = (2*y_mid + y_tower)/3 [m]
    
    double radiusMm = 0.0;   // Raio físico do subcondutor [mm]
    double gmrMm = 0.0;      // GMR individual do subcondutor [mm]
    double r_ac = 0.0;       // Resistência CA corrigida [ohm/km]
    double x_int = 0.0;      // Reatância interna [ohm/km]
};

class LineGeometry {
public:
    // Expande todas as fases (incluindo feixes) e cabos para-raios em condutores físicos individuais
    static std::vector<PhysicalConductor> expandConductors(const LineSystemInput& input);

    // Calcula a flecha mecânica d_flecha = (w_r * L²) / (8 * T) se não informada
    static double calculateMechanicalSag(double weightPerMeter, double diameterMm,
                                         const EnvironmentalData& env);

    // Matriz de distâncias diretas condutor-condutor: d_ij = sqrt((xi - xj)² + (hi - hj)²) [m]
    static RealMatrix buildDirectDistanceMatrix(const std::vector<PhysicalConductor>& conds);

    // Matriz de distâncias ao condutor imagem: D_ij = sqrt((xi - xj)² + (hi + hj)²) [m]
    static RealMatrix buildImageDistanceMatrix(const std::vector<PhysicalConductor>& conds);
};

} // namespace LineParameters

#endif // LINE_GEOMETRY_HPP
