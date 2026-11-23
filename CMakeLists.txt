cmake_minimum_required(VERSION 3.12)
project(LineParametersCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Solicita a versão estática do wxWidgets (sem DLLs externas)
set(wxWidgets_USE_STATIC ON)

if(MINGW)
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base adv)
include(${wxWidgets_USE_FILE})

set(CORE_SOURCES
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

add_executable(line_calc_gui WIN32
    ${CORE_SOURCES}
    src/AppMain.cpp
    src/MainFrameBase.h
    src/MainFrameBase.cpp
    src/MainFrame.h
    src/MainFrame.cpp
    src/ConductorCatalogManager.hpp
    src/ConductorCatalogManager.cpp
    src/ConductorPickerDialog.hpp
    src/ConductorPickerDialog.cpp
    src/ConductorPickerDialogBase.cpp
    src/ConductorPickerDialogBase.h    
    src/resources.rc
)

target_include_directories(line_calc_gui PRIVATE src)
target_link_libraries(line_calc_gui PRIVATE ${wxWidgets_LIBRARIES})