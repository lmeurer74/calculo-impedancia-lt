#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "MainFrameBase.h"
#include "InputData.hpp"
#include "LineParameters.hpp"
#include "SymmetricalComponents.hpp"
#include "ReportGenerator.hpp"
#include "ConductorPickerDialog.hpp"

class MainFrame : public MainFrameBase
{
public:
    MainFrame(wxWindow* parent = nullptr);
    virtual ~MainFrame() = default;

protected:
    // Handlers dos botões
    void OnAddPhase(wxCommandEvent& event);
    void OnAddGroundWire(wxCommandEvent& event);
    void OnRemoveRow(wxCommandEvent& event);
    void OnLoadExample(wxCommandEvent& event);
    void OnCalculate(wxCommandEvent& event);
    void OnExportReport(wxCommandEvent& event);
    void OnClearReport(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnGridCellDoubleClick(wxGridEvent& event);

private:
    void SetupGrid();
    void AppendGridRow(const wxString& tipo, int circuito, const wxString& id,
                       double xTorre, double yTorre, bool temYMeio, double yMeio, double flecha,
                       int nSub, double espac, double angulo, const wxString& arranjoOuAterr,
                       const wxString& nomeCabo, double rdc20, double diamMm, double gmrMm,
                       double pesoKgM, double alpha20, double skinRatio);
    void ApplyConductorToRow(int row, const LineParameters::ConductorCatalog& cond);
    LineParameters::LineSystemInput ReadInputFromGUI();
    static wxString FormatDouble(double val, int decimals);
    static double ParseDouble(const wxString& str, double defaultVal = 0.0);
    static int ParseInt(const wxString& str, int defaultVal = 1);
};

#endif // MAINFRAME_H