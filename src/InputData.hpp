#ifndef INPUT_DATA_HPP
#define INPUT_DATA_HPP

#include "Types.hpp"
#include <string>
#include <vector>

namespace LineParameters {

// Dados de catálogo / fabricante de um condutor (Entradas Primárias)
struct ConductorCatalog {
    std::string codeName;        // Ex: "Drake", "Grosbeak", "Rail"
    double r_dc_20 = 0.0;        // Resistência CC a 20°C [ohm/km]
    double outsideDiameter = 0.0;// Diâmetro externo total [mm]
    double gmr = 0.0;            // Raio Médio Geométrico individual (GMR) [mm]
    double weightPerMeter = 0.0;  // Peso linear do cabo [kg/m]
    double alpha20 = 0.00403;    // Coeficiente térmico [1/°C] (Alumínio ~ 0.00403)
    double skinRatio = 0.5;      // T/D para efeito pelicular (0.5 = maciço / tubular)
};

// Configuração geométrica do feixe de subcondutores de uma fase
struct BundleConfig {
    int numSubconductors = 1;    // 1, 2, 3, 4, ...
    double spacing = 0.45;       // Espaçamento entre subcondutores adjacentes [m]
    double angleDeg = 0.0;       // Ângulo de rotação em relação à horizontal [graus]
    BundleArrangement arrangement = BundleArrangement::Single;
};

// Posição de uma fase (ou polo) na torre
struct PhaseData {
    std::string phaseId;         // "A1", "B1", "C1", "A2", "B2", "C2"
    int circuitId = 1;           // 1 ou 2 (para circuito duplo)
    double x_tower = 0.0;        // Posição horizontal na torre [m] (centro torre = 0)
    double y_tower = 0.0;        // Altura de fixação na torre [m]
    
    // Condicional de flecha: prioriza projeto se disponível
    bool hasMidSpanHeight = false;
    double y_midSpan = 0.0;      // Altura no meio do vão [m]
    double sagProject = 0.0;     // Flecha do vão [m]

    BundleConfig bundle;
    ConductorCatalog conductor;
};

// Dados de um cabo para-raios (cabo terra / shield wire)
struct GroundWireData {
    std::string wireId;          // "GW1", "GW2", "OPGW"
    double x_tower = 0.0;        // Posição horizontal na torre [m]
    double y_tower = 0.0;        // Altura de fixação na torre [m]

    bool hasMidSpanHeight = false;
    double y_midSpan = 0.0;
    double sagProject = 0.0;

    GroundWireGrounding grounding = GroundWireGrounding::Continuous;
    ConductorCatalog conductor;
};

// Parâmetros Ambientais e Mecânicos (usados caso a flecha precise ser calculada mecanicamente)
struct EnvironmentalData {
    double operatingTemperature = 75.0; // Temperatura do condutor [°C]
    double ambientTemperature = 25.0;   // Temperatura ambiente [°C]
    double windPressure = 0.0;          // Pressão horizontal do vento [kg/m²]
    double iceThickness = 0.0;          // Espessura de gelo [mm]
    double horizontalTension = 2000.0;  // Tração mecânica horizontal [kgf]
    double spanLength = 300.0;          // Vão horizontal [m]
};

// Bases do Sistema Elétrico e Condições Gerais da Linha
struct LineSystemInput {
    std::string lineName = "Linha de Transmissao";
    double frequency = 60.0;            // Frequência [Hz]
    double earthResistivity = 100.0;    // Resistividade do solo [ohm.m]
    double lineLengthKm = 100.0;        // Comprimento total da linha [km]

    // Bases para cálculo em pu
    double vBaseKv = 230.0;             // Tensão base de linha [kV]
    double sBaseMva = 100.0;            // Potência base trifásica [MVA]

    TranspositionMode transposition = TranspositionMode::Untransposed;

    EnvironmentalData environment;
    std::vector<PhaseData> phases;
    std::vector<GroundWireData> groundWires;
};

} // namespace LineParameters

#endif // INPUT_DATA_HPP
