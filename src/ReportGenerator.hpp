#ifndef REPORT_GENERATOR_HPP
#define REPORT_GENERATOR_HPP

#include "Types.hpp"
#include "InputData.hpp"
#include "LineParameters.hpp"
#include "SymmetricalComponents.hpp"
#include <string>

namespace LineParameters {

class ReportGenerator {
public:
    static std::string generateReport(const LineSystemInput& input,
                                      const CalculatedMatrices& mats,
                                      const SequenceResults& seq);

    static void printReport(const LineSystemInput& input,
                            const CalculatedMatrices& mats,
                            const SequenceResults& seq);
};

} // namespace LineParameters

#endif // REPORT_GENERATOR_HPP
