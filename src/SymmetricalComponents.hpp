#ifndef SYMMETRICAL_COMPONENTS_HPP
#define SYMMETRICAL_COMPONENTS_HPP

#include "Types.hpp"
#include "InputData.hpp"

namespace LineParameters {

struct SequenceResults {
    // Sequência Positiva (1) e Zero (0) do circuito
    Complex Z1;         // [ohm/km]
    Complex Z0;         // [ohm/km]
    double C1 = 0.0;    // [F/km]
    double C0 = 0.0;    // [F/km]
    Complex Y1;         // [S/km]
    Complex Y0;         // [S/km]

    // Para circuito duplo na mesma estrutura:
    bool isDoubleCircuit = false;
    Complex Z1_c2;      // Sequência positiva do circuito 2 [ohm/km]
    Complex Z0_c2;      // Sequência zero do circuito 2 [ohm/km]
    Complex Z0_mutual;  // Acoplamento mútuo de sequência zero entre circuitos [ohm/km]
    double C0_mutual = 0.0; // Capacitância mútua de sequência zero [F/km]
    Complex Y0_mutual;  // Admitância mútua de sequência zero [S/km]

    // Matrizes completas de componentes simétricas
    ComplexMatrix Z_seq; // Matriz [Z_012]
    ComplexMatrix Y_seq; // Matriz [Y_012]
};

class SymmetricalComponentsEngine {
public:
    // Converte matrizes de fase [Z_phase] e [Y_phase] em componentes simétricas e parâmetros médios
    static SequenceResults analyze(const ComplexMatrix& Z_phase,
                                   const RealMatrix& C_phase,
                                   const ComplexMatrix& Y_phase,
                                   const LineSystemInput& input);

private:
    static ComplexMatrix multiply(const ComplexMatrix& A, const ComplexMatrix& B);
};

} // namespace LineParameters

#endif // SYMMETRICAL_COMPONENTS_HPP
