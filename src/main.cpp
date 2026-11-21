#include "InputData.hpp"
#include "LineParameters.hpp"
#include "SymmetricalComponents.hpp"
#include "ReportGenerator.hpp"
#include <iostream>

using namespace LineParameters;

int main() {
    std::cout << "Inicializando Calculo de Parametros Eletricos de Linhas de Transmissao...\n\n";

    // 1. Definição do Cabo Condutor de Fase (Drake - 795 kcmil ACSR 26/7)
    ConductorCatalog acsrDrake;
    acsrDrake.codeName = "Drake (795 kcmil ACSR 26/7)";
    acsrDrake.r_dc_20 = 0.0722;       // ohm/km
    acsrDrake.outsideDiameter = 28.14; // mm
    acsrDrake.gmr = 11.37;            // mm
    acsrDrake.weightPerMeter = 1.628;  // kg/m
    acsrDrake.alpha20 = 0.00403;
    acsrDrake.skinRatio = 0.5;

    // 2. Definição do Cabo Para-Raios (Aço EHS 3/8")
    ConductorCatalog shieldSteel;
    shieldSteel.codeName = "Steel 3/8 EHS";
    shieldSteel.r_dc_20 = 3.50;        // ohm/km
    shieldSteel.outsideDiameter = 9.14;// mm
    shieldSteel.gmr = 1.05;            // mm
    shieldSteel.weightPerMeter = 0.407;// kg/m
    shieldSteel.alpha20 = 0.0045;
    shieldSteel.skinRatio = 0.5;

    // 3. Montagem do Caso de Teste: Linha 230 kV Circuito Simples com Feixe Duplo (2 x Drake)
    LineSystemInput line;
    line.lineName = "LT 230 kV Exemplo Didatico (2x Drake, 2x Para-raios)";
    line.frequency = 60.0;             // Hz
    line.earthResistivity = 100.0;     // ohm.m
    line.lineLengthKm = 120.0;         // 120 km
    line.vBaseKv = 230.0;              // 230 kV
    line.sBaseMva = 100.0;             // 100 MVA
    line.transposition = TranspositionMode::Transposed;

    // Condições ambientais e mecânicas
    line.environment.operatingTemperature = 75.0; // °C
    line.environment.ambientTemperature = 25.0;   // °C
    line.environment.spanLength = 350.0;          // 350 m
    line.environment.horizontalTension = 2500.0;  // kgf
    line.environment.windPressure = 0.0;

    // Configuração do Feixe Duplo (2 subcondutores separados por 45 cm horizontalmente)
    BundleConfig bundle2x;
    bundle2x.numSubconductors = 2;
    bundle2x.spacing = 0.45;           // 45 cm
    bundle2x.angleDeg = 0.0;           // Horizontal
    bundle2x.arrangement = BundleArrangement::Horizontal;

    // Fases A, B, C dispostas horizontalmente na torre
    // Fase A: x = -7.0 m, y_torre = 22.0 m
    PhaseData phaseA;
    phaseA.phaseId = "A";
    phaseA.circuitId = 1;
    phaseA.x_tower = -7.0;
    phaseA.y_tower = 22.0;
    phaseA.hasMidSpanHeight = true;
    phaseA.y_midSpan = 14.0; // Altura no meio do vão conhecida do projeto
    phaseA.bundle = bundle2x;
    phaseA.conductor = acsrDrake;
    line.phases.push_back(phaseA);

    // Fase B: x = 0.0 m, y_torre = 22.0 m
    PhaseData phaseB;
    phaseB.phaseId = "B";
    phaseB.circuitId = 1;
    phaseB.x_tower = 0.0;
    phaseB.y_tower = 22.0;
    phaseB.hasMidSpanHeight = true;
    phaseB.y_midSpan = 14.0;
    phaseB.bundle = bundle2x;
    phaseB.conductor = acsrDrake;
    line.phases.push_back(phaseB);

    // Fase C: x = +7.0 m, y_torre = 22.0 m
    PhaseData phaseC;
    phaseC.phaseId = "C";
    phaseC.circuitId = 1;
    phaseC.x_tower = 7.0;
    phaseC.y_tower = 22.0;
    phaseC.hasMidSpanHeight = true;
    phaseC.y_midSpan = 14.0;
    phaseC.bundle = bundle2x;
    phaseC.conductor = acsrDrake;
    line.phases.push_back(phaseC);

    // Dois cabos para-raios contínuos no topo da estrutura:
    // GW1: x = -4.5 m, y = 29.0 m
    GroundWireData gw1;
    gw1.wireId = "GW1";
    gw1.x_tower = -4.5;
    gw1.y_tower = 29.0;
    gw1.hasMidSpanHeight = true;
    gw1.y_midSpan = 23.0;
    gw1.grounding = GroundWireGrounding::Continuous;
    gw1.conductor = shieldSteel;
    line.groundWires.push_back(gw1);

    // GW2: x = +4.5 m, y = 29.0 m
    GroundWireData gw2;
    gw2.wireId = "GW2";
    gw2.x_tower = 4.5;
    gw2.y_tower = 29.0;
    gw2.hasMidSpanHeight = true;
    gw2.y_midSpan = 23.0;
    gw2.grounding = GroundWireGrounding::Continuous;
    gw2.conductor = shieldSteel;
    line.groundWires.push_back(gw2);

    // 4. Execução dos Cálculos
    CalculatedMatrices matrices = LineParametersEngine::compute(line);
    SequenceResults seq = SymmetricalComponentsEngine::analyze(matrices.Z_phase,
                                                              matrices.C_phase,
                                                              matrices.Y_phase,
                                                              line);

    // 5. Apresentação do Relatório Completo
    ReportGenerator::printReport(line, matrices, seq);

    return 0;
}
