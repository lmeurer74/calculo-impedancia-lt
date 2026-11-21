#include "SymmetricalComponents.hpp"
#include <cmath>

namespace LineParameters {

ComplexMatrix SymmetricalComponentsEngine::multiply(const ComplexMatrix& A, const ComplexMatrix& B) {
    size_t rowsA = A.size();
    size_t colsA = A[0].size();
    size_t colsB = B[0].size();
    ComplexMatrix C = createComplexMatrix(rowsA, colsB, Complex(0.0, 0.0));
    for (size_t i = 0; i < rowsA; ++i) {
        for (size_t j = 0; j < colsB; ++j) {
            for (size_t k = 0; k < colsA; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C;
}

SequenceResults SymmetricalComponentsEngine::analyze(const ComplexMatrix& Z_phase,
                                                    const RealMatrix& C_phase,
                                                    const ComplexMatrix& Y_phase,
                                                    const LineSystemInput& input) {
    SequenceResults res;
    size_t nPhases = Z_phase.size();
    double omega = 2.0 * PI * input.frequency;

    // Operador de Fortescue: a = e^(j * 120°) = -0.5 + j * sqrt(3)/2
    Complex a(-0.5, std::sqrt(3.0) / 2.0);
    Complex a2 = a * a;

    if (nPhases == 3) {
        // Circuito simples trifásico
        res.isDoubleCircuit = false;

        ComplexMatrix As = createComplexMatrix(3, 3);
        As[0][0] = 1.0; As[0][1] = 1.0; As[0][2] = 1.0;
        As[1][0] = 1.0; As[1][1] = a2;  As[1][2] = a;
        As[2][0] = 1.0; As[2][1] = a;   As[2][2] = a2;

        ComplexMatrix As_inv = createComplexMatrix(3, 3);
        Complex factor = 1.0 / 3.0;
        As_inv[0][0] = factor; As_inv[0][1] = factor; As_inv[0][2] = factor;
        As_inv[1][0] = factor; As_inv[1][1] = a * factor; As_inv[1][2] = a2 * factor;
        As_inv[2][0] = factor; As_inv[2][1] = a2 * factor; As_inv[2][2] = a * factor;

        res.Z_seq = multiply(As_inv, multiply(Z_phase, As));
        res.Y_seq = multiply(As_inv, multiply(Y_phase, As));

        if (input.transposition == TranspositionMode::Transposed) {
            Complex Zs = (Z_phase[0][0] + Z_phase[1][1] + Z_phase[2][2]) / 3.0;
            Complex Zm = (Z_phase[0][1] + Z_phase[1][2] + Z_phase[2][0] +
                          Z_phase[1][0] + Z_phase[2][1] + Z_phase[0][2]) / 6.0;

            res.Z1 = Zs - Zm;
            res.Z0 = Zs + 2.0 * Zm;

            double Cs = (C_phase[0][0] + C_phase[1][1] + C_phase[2][2]) / 3.0;
            double Cm = (C_phase[0][1] + C_phase[1][2] + C_phase[2][0] +
                         C_phase[1][0] + C_phase[2][1] + C_phase[0][2]) / 6.0;

            res.C1 = Cs - Cm;
            res.C0 = Cs + 2.0 * Cm;
        } else {
            res.Z0 = res.Z_seq[0][0];
            res.Z1 = res.Z_seq[1][1];

            res.Y0 = res.Y_seq[0][0];
            res.Y1 = res.Y_seq[1][1];

            res.C0 = std::abs(res.Y0.imag()) / omega;
            res.C1 = std::abs(res.Y1.imag()) / omega;
        }

        res.Y1 = Complex(0.0, omega * res.C1);
        res.Y0 = Complex(0.0, omega * res.C0);

    } else if (nPhases == 6) {
        // Circuito Duplo na mesma estrutura (C1: fases 0,1,2 e C2: fases 3,4,5)
        res.isDoubleCircuit = true;

        ComplexMatrix Z11 = createComplexMatrix(3, 3);
        ComplexMatrix Z22 = createComplexMatrix(3, 3);
        ComplexMatrix Z12 = createComplexMatrix(3, 3);

        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                Z11[i][j] = Z_phase[i][j];
                Z22[i][j] = Z_phase[3 + i][3 + j];
                Z12[i][j] = Z_phase[i][3 + j];
            }
        }

        ComplexMatrix As = createComplexMatrix(3, 3);
        As[0][0] = 1.0; As[0][1] = 1.0; As[0][2] = 1.0;
        As[1][0] = 1.0; As[1][1] = a2;  As[1][2] = a;
        As[2][0] = 1.0; As[2][1] = a;   As[2][2] = a2;

        ComplexMatrix As_inv = createComplexMatrix(3, 3);
        Complex factor = 1.0 / 3.0;
        As_inv[0][0] = factor; As_inv[0][1] = factor; As_inv[0][2] = factor;
        As_inv[1][0] = factor; As_inv[1][1] = a * factor; As_inv[1][2] = a2 * factor;
        As_inv[2][0] = factor; As_inv[2][1] = a2 * factor; As_inv[2][2] = a * factor;

        ComplexMatrix Z11_seq = multiply(As_inv, multiply(Z11, As));
        ComplexMatrix Z22_seq = multiply(As_inv, multiply(Z22, As));
        ComplexMatrix Z12_seq = multiply(As_inv, multiply(Z12, As));

        res.Z1 = Z11_seq[1][1];
        res.Z0 = Z11_seq[0][0];

        res.Z1_c2 = Z22_seq[1][1];
        res.Z0_c2 = Z22_seq[0][0];

        // Acoplamento Mútuo de Sequência Zero entre Circuitos
        res.Z0_mutual = Z12_seq[0][0];

        double Cs = (C_phase[0][0] + C_phase[1][1] + C_phase[2][2]) / 3.0;
        double Cm = (C_phase[0][1] + C_phase[1][2] + C_phase[2][0] +
                     C_phase[1][0] + C_phase[2][1] + C_phase[0][2]) / 6.0;
        res.C1 = Cs - Cm;
        res.C0 = Cs + 2.0 * Cm;
        res.Y1 = Complex(0.0, omega * res.C1);
        res.Y0 = Complex(0.0, omega * res.C0);

        double Cm_circ = 0.0;
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 3; j < 6; ++j) {
                Cm_circ += std::abs(C_phase[i][j]);
            }
        }
        res.C0_mutual = Cm_circ / 3.0;
        res.Y0_mutual = Complex(0.0, omega * res.C0_mutual);
    }

    return res;
}

} // namespace LineParameters
