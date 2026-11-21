#include "ReportGenerator.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace LineParameters {

std::string ReportGenerator::generateReport(const LineSystemInput& input,
                                            const CalculatedMatrices& mats,
                                            const SequenceResults& seq) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);

    double Vb = input.vBaseKv;       // kV
    double Sb = input.sBaseMva;      // MVA
    double Zb = (Vb * Vb) / Sb;      // ohm
    double Yb = 1.0 / Zb;            // S
    double L = input.lineLengthKm;   // km

    oss << "========================================================================================\n";
    oss << "             RELATORIO DE PARAMETROS ELETRICOS DE LINHA DE TRANSMISSAO                  \n";
    oss << "========================================================================================\n";
    oss << "Linha: " << input.lineName << "\n";
    oss << "Frequencia: " << input.frequency << " Hz | Resistividade do Solo: " << input.earthResistivity << " ohm.m\n";
    oss << "Comprimento: " << L << " km | Vbase: " << Vb << " kV | Sbase: " << Sb << " MVA\n";
    oss << "Impedancia Base (Zbase): " << Zb << " ohm | Admitancia Base (Ybase): " << Yb * 1e6 << " uS\n";
    oss << "Regime de Transposicao: " << (input.transposition == TranspositionMode::Transposed ? "TRANSPOSTA" : "NAO TRANSPOSTA") << "\n";
    oss << "----------------------------------------------------------------------------------------\n\n";

    oss << "1. PARAMETROS DE SEQUENCIA UNITARIOS (POR QUILOMETRO):\n";
    oss << "----------------------------------------------------------------------------------------\n";
    oss << "SEQUENCIA POSITIVA (Z1, Y1, C1):\n";
    oss << "  * Z1 (ohm/km): " << seq.Z1.real() << " + j " << seq.Z1.imag() << " ohm/km\n";
    oss << "  * Z1 (pu/km):  " << (seq.Z1.real() / Zb) << " + j " << (seq.Z1.imag() / Zb) << " pu/km\n";
    oss << "  * C1:          " << seq.C1 * 1e9 << " nF/km  (" << seq.C1 * 1e6 << " uF/km)\n";
    oss << "  * Y1 (uS/km):  j " << seq.Y1.imag() * 1e6 << " uS/km\n";
    oss << "  * Y1 (pu/km):  j " << (seq.Y1.imag() / Yb) << " pu/km\n\n";

    oss << "SEQUENCIA ZERO (Z0, Y0, C0):\n";
    oss << "  * Z0 (ohm/km): " << seq.Z0.real() << " + j " << seq.Z0.imag() << " ohm/km\n";
    oss << "  * Z0 (pu/km):  " << (seq.Z0.real() / Zb) << " + j " << (seq.Z0.imag() / Zb) << " pu/km\n";
    oss << "  * C0:          " << seq.C0 * 1e9 << " nF/km  (" << seq.C0 * 1e6 << " uF/km)\n";
    oss << "  * Y0 (uS/km):  j " << seq.Y0.imag() * 1e6 << " uS/km\n";
    oss << "  * Y0 (pu/km):  j " << (seq.Y0.imag() / Yb) << " pu/km\n\n";

    if (seq.isDoubleCircuit) {
        oss << "ACOPLAMENTO MUTUO ENTRE CIRCUITOS (SEQUENCIA ZERO Z0m, Y0m, C0m):\n";
        oss << "  * Z0m (ohm/km): " << seq.Z0_mutual.real() << " + j " << seq.Z0_mutual.imag() << " ohm/km\n";
        oss << "  * Z0m (pu/km):  " << (seq.Z0_mutual.real() / Zb) << " + j " << (seq.Z0_mutual.imag() / Zb) << " pu/km\n";
        oss << "  * C0m:          " << seq.C0_mutual * 1e9 << " nF/km\n";
        oss << "  * Y0m (uS/km):  j " << seq.Y0_mutual.imag() * 1e6 << " uS/km\n";
        oss << "  * Y0m (pu/km):  j " << (seq.Y0_mutual.imag() / Yb) << " pu/km\n\n";
    }

    oss << "----------------------------------------------------------------------------------------\n";
    oss << "2. PARAMETROS TOTAIS DA LINHA (COMPRIMENTO L = " << L << " km):\n";
    oss << "----------------------------------------------------------------------------------------\n";
    Complex Z1_tot = seq.Z1 * L;
    Complex Z0_tot = seq.Z0 * L;
    double C1_tot = seq.C1 * L;
    double C0_tot = seq.C0 * L;
    Complex Y1_tot = seq.Y1 * L;
    Complex Y0_tot = seq.Y0 * L;

    oss << "SEQUENCIA POSITIVA TOTAL:\n";
    oss << "  * Z1_total (ohm): " << Z1_tot.real() << " + j " << Z1_tot.imag() << " ohm\n";
    oss << "  * Z1_total (pu):  " << (Z1_tot.real() / Zb) << " + j " << (Z1_tot.imag() / Zb) << " pu\n";
    oss << "  * C1_total (uF):  " << C1_tot * 1e6 << " uF\n";
    oss << "  * Y1_total (S):   j " << Y1_tot.imag() << " S\n";
    oss << "  * Y1_total (pu):  j " << (Y1_tot.imag() / Yb) << " pu\n\n";

    oss << "SEQUENCIA ZERO TOTAL:\n";
    oss << "  * Z0_total (ohm): " << Z0_tot.real() << " + j " << Z0_tot.imag() << " ohm\n";
    oss << "  * Z0_total (pu):  " << (Z0_tot.real() / Zb) << " + j " << (Z0_tot.imag() / Zb) << " pu\n";
    oss << "  * C0_total (uF):  " << C0_tot * 1e6 << " uF\n";
    oss << "  * Y0_total (S):   j " << Y0_tot.imag() << " S\n";
    oss << "  * Y0_total (pu):  j " << (Y0_tot.imag() / Yb) << " pu\n\n";

    if (seq.isDoubleCircuit) {
        Complex Z0m_tot = seq.Z0_mutual * L;
        Complex Y0m_tot = seq.Y0_mutual * L;
        oss << "MUTUA DE SEQUENCIA ZERO TOTAL ENTRE CIRCUITOS:\n";
        oss << "  * Z0m_total (ohm): " << Z0m_tot.real() << " + j " << Z0m_tot.imag() << " ohm\n";
        oss << "  * Z0m_total (pu):  " << (Z0m_tot.real() / Zb) << " + j " << (Z0m_tot.imag() / Zb) << " pu\n";
        oss << "  * C0m_total (uF):  " << seq.C0_mutual * L * 1e6 << " uF\n";
        oss << "  * Y0m_total (pu):  j " << (Y0m_tot.imag() / Yb) << " pu\n\n";
    }

    oss << "----------------------------------------------------------------------------------------\n";
    oss << "3. MATRIZES REDUZIDAS DE FASE [Z_fase] e [C_fase] (Apos Kron e Feixes):\n";
    oss << "----------------------------------------------------------------------------------------\n";
    size_t n = mats.Z_phase.size();
    oss << "Matriz [Z_fase] [ohm/km]:\n";
    for (size_t i = 0; i < n; ++i) {
        oss << "  | ";
        for (size_t j = 0; j < n; ++j) {
            oss << std::setw(9) << mats.Z_phase[i][j].real() << " + j"
                << std::setw(9) << mats.Z_phase[i][j].imag() << "  ";
        }
        oss << "|\n";
    }

    oss << "\nMatriz [C_fase] [nF/km]:\n";
    for (size_t i = 0; i < n; ++i) {
        oss << "  | ";
        for (size_t j = 0; j < n; ++j) {
            oss << std::setw(12) << mats.C_phase[i][j] * 1e9 << "  ";
        }
        oss << "|\n";
    }
    oss << "========================================================================================\n";

    return oss.str();
}

void ReportGenerator::printReport(const LineSystemInput& input,
                                 const CalculatedMatrices& mats,
                                 const SequenceResults& seq) {
    std::cout << generateReport(input, mats, seq);
}

} // namespace LineParameters
