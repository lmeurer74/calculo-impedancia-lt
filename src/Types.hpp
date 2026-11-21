#ifndef TYPES_HPP
#define TYPES_HPP

#include <complex>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace LineParameters {

constexpr double PI = 3.14159265358979323846;
constexpr double MU_0 = 4.0 * PI * 1e-7;       // Permeabilidade do vácuo [H/m]
constexpr double EPS_0 = 8.8541878128e-12;     // Permissividade do vácuo [F/m]

// Definições de precisão e tipos numéricos
using Complex = std::complex<double>;
using RealMatrix = std::vector<std::vector<double>>;
using ComplexMatrix = std::vector<std::vector<Complex>>;

// Enumerações para configurações de Linha
enum class BundleArrangement {
    Single,         // 1 condutor por fase
    Horizontal,     // 2 condutores horizontais / inclinados
    Triangular,     // 3 condutores em triângulo equilátero
    Square,         // 4 condutores em quadrado
    Circular        // N condutores uniformemente distribuídos em círculo
};

enum class TranspositionMode {
    Untransposed,   // Linha real não transposta (mantém desbalanceamento)
    Transposed      // Linha idealmente transposta (médias diagonais e mútuas)
};

enum class GroundWireGrounding {
    Continuous,     // Aterrado continuamente em todas as torres (redução de Kron em Z e P)
    Segmented       // Seccionado/isolado (ignorado em Z, considerado em P)
};

// Funções utilitárias matriciais para C++ padrão (sem dependências externas)
inline ComplexMatrix createComplexMatrix(size_t rows, size_t cols, Complex init = Complex(0.0, 0.0)) {
    return ComplexMatrix(rows, std::vector<Complex>(cols, init));
}

inline RealMatrix createRealMatrix(size_t rows, size_t cols, double init = 0.0) {
    return RealMatrix(rows, std::vector<double>(cols, init));
}

// Inversão de matriz complexa via Eliminação de Gauss-Jordan com pivoteamento parcial
inline bool invertMatrix(const ComplexMatrix& A, ComplexMatrix& A_inv) {
    size_t n = A.size();
    if (n == 0 || A[0].size() != n) return false;

    // Matriz aumentada [A | I]
    std::vector<std::vector<Complex>> aug(n, std::vector<Complex>(2 * n, Complex(0.0, 0.0)));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            aug[i][j] = A[i][j];
        }
        aug[i][n + i] = Complex(1.0, 0.0);
    }

    for (size_t i = 0; i < n; ++i) {
        // Encontrar pivô máximo
        size_t maxRow = i;
        double maxVal = std::abs(aug[i][i]);
        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(aug[k][i]) > maxVal) {
                maxVal = std::abs(aug[k][i]);
                maxRow = k;
            }
        }

        if (maxVal < 1e-18) {
            return false; // Matriz singular ou mal condicionada
        }

        // Troca de linhas
        std::swap(aug[i], aug[maxRow]);

        // Normalização da linha pivô
        Complex pivot = aug[i][i];
        for (size_t j = i; j < 2 * n; ++j) {
            aug[i][j] /= pivot;
        }

        // Eliminação nas outras linhas
        for (size_t k = 0; k < n; ++k) {
            if (k != i) {
                Complex factor = aug[k][i];
                for (size_t j = i; j < 2 * n; ++j) {
                    aug[k][j] -= factor * aug[i][j];
                }
            }
        }
    }

    A_inv = createComplexMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            A_inv[i][j] = aug[i][n + j];
        }
    }
    return true;
}

// Inversão de matriz real via Gauss-Jordan
inline bool invertMatrix(const RealMatrix& A, RealMatrix& A_inv) {
    size_t n = A.size();
    if (n == 0 || A[0].size() != n) return false;

    std::vector<std::vector<double>> aug(n, std::vector<double>(2 * n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            aug[i][j] = A[i][j];
        }
        aug[i][n + i] = 1.0;
    }

    for (size_t i = 0; i < n; ++i) {
        size_t maxRow = i;
        double maxVal = std::abs(aug[i][i]);
        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(aug[k][i]) > maxVal) {
                maxVal = std::abs(aug[k][i]);
                maxRow = k;
            }
        }

        if (maxVal < 1e-18) return false;

        std::swap(aug[i], aug[maxRow]);

        double pivot = aug[i][i];
        for (size_t j = i; j < 2 * n; ++j) {
            aug[i][j] /= pivot;
        }

        for (size_t k = 0; k < n; ++k) {
            if (k != i) {
                double factor = aug[k][i];
                for (size_t j = i; j < 2 * n; ++j) {
                    aug[k][j] -= factor * aug[i][j];
                }
            }
        }
    }

    A_inv = createRealMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            A_inv[i][j] = aug[i][n + j];
        }
    }
    return true;
}

} // namespace LineParameters

#endif // TYPES_HPP
