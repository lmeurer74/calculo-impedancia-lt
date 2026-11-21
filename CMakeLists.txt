cmake_minimum_required(VERSION 3.12)
project(LineParametersCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/Types.hpp
    src/InputData.hpp
    src/ConductorModel.hpp
    src/ConductorModel.cpp
    src/LineGeometry.hpp
    src/LineGeometry.cpp
    src/LineParameters.hpp
    src/LineParameters.cpp
    src/SymmetricalComponents.hpp
    src/SymmetricalComponents.cpp
    src/ReportGenerator.hpp
    src/ReportGenerator.cpp
)

add_executable(line_calc ${SOURCES})
