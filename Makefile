CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc
LDFLAGS = -static -static-libgcc -static-libstdc++

WX_CXXFLAGS := $(shell wx-config --static=yes --cxxflags)
WX_LIBS := $(shell wx-config --static=yes --libs core,base,adv)

SRC_DIR = src
BUILD_DIR = build

# Fontes do motor numérico
CORE_SRCS = $(SRC_DIR)/ConductorModel.cpp \
            $(SRC_DIR)/LineGeometry.cpp \
            $(SRC_DIR)/LineParameters.cpp \
            $(SRC_DIR)/SymmetricalComponents.cpp \
            $(SRC_DIR)/ReportGenerator.cpp

# Fontes da interface gráfica
GUI_SRCS = $(CORE_SRCS) \
           $(SRC_DIR)/AppMain.cpp \
           $(SRC_DIR)/MainFrameBase.cpp \
           $(SRC_DIR)/MainFrame.cpp \
           $(SRC_DIR)/ConductorCatalogManager.cpp \
           $(SRC_DIR)/ConductorPickerDialog.cpp \
		   $(SRC_DIR)/ConductorPickerDialogBase.cpp

GUI_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/gui_%.o, $(GUI_SRCS))
TARGET_GUI = $(BUILD_DIR)/line_calc_gui.exe

# Arquivo de recurso do ícone do Windows
RES_OBJ = $(BUILD_DIR)/resources.o

all: $(TARGET_GUI)

gui: $(TARGET_GUI)

# Linkagem final incluindo o arquivo de recurso (RES_OBJ)
$(TARGET_GUI): $(GUI_OBJS) $(RES_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(WX_LIBS) $(LDFLAGS) -mwindows

$(BUILD_DIR)/gui_%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) -c $< -o $@

# Compila o .rc em um .o binário nativo do Windows
$(RES_OBJ): $(SRC_DIR)/resources.rc $(SRC_DIR)/app_icon.ico
	@mkdir -p $(BUILD_DIR)
	windres $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all gui clean