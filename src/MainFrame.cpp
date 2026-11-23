#include "MainFrame.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/gdicmn.h>
#include <algorithm>
#include <vector>
#include <wx/numformatter.h>
#include <wx/aboutdlg.h>
#include <wx/icon.h>
#include "ConductorCatalogManager.hpp"
#include "ConductorPickerDialog.hpp"

MainFrame::MainFrame(wxWindow* parent)
    : MainFrameBase(parent)
{
    // Carrega o ícone embutido no recurso do executável do Windows
    SetIcon(wxICON(IDI_APP_ICON));

    // Vincula os eventos com segurança...
    if (btn_addPhase)      btn_addPhase->Bind(wxEVT_BUTTON, &MainFrame::OnAddPhase, this);

    SetupGrid();

    // Garante que o quadro de relatório e a caixa de texto ocupem toda a largura disponível
    if (txt_reportOutput && txt_reportOutput->GetContainingSizer()) {
        wxSizer* reportBoxSizer = txt_reportOutput->GetContainingSizer();
        wxSizerItem* txtItem = reportBoxSizer->GetItem(txt_reportOutput);
        if (txtItem) {
            txtItem->SetFlag(wxEXPAND | wxALL);
            txtItem->SetProportion(1);
        }
        wxWindow* parentPanel = txt_reportOutput->GetParent();
        if (parentPanel && parentPanel->GetSizer()) {
            wxSizerItem* boxItem = parentPanel->GetSizer()->GetItem(reportBoxSizer);
            if (boxItem) {
                boxItem->SetFlag(wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM);
                boxItem->SetProportion(1);
            }
        }
    }

    // Vincula os eventos diretamente aos botões
    btn_addPhase->Bind(wxEVT_BUTTON, &MainFrame::OnAddPhase, this);
    btn_addGroundWire->Bind(wxEVT_BUTTON, &MainFrame::OnAddGroundWire, this);
    btn_removeRow->Bind(wxEVT_BUTTON, &MainFrame::OnRemoveRow, this);
    btn_loadExample->Bind(wxEVT_BUTTON, &MainFrame::OnLoadExample, this);
    btn_calculate->Bind(wxEVT_BUTTON, &MainFrame::OnCalculate, this);
    btn_exportReport->Bind(wxEVT_BUTTON, &MainFrame::OnExportReport, this);
    btn_clearReport->Bind(wxEVT_BUTTON, &MainFrame::OnClearReport, this);
    btn_exit->Bind(wxEVT_BUTTON, &MainFrame::OnExit, this);
    btn_about->Bind(wxEVT_BUTTON, &MainFrame::OnAbout, this);
    grid_conductors->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &MainFrame::OnGridCellDoubleClick, this);

    // Carrega o caso didático LT 230 kV (2x Drake) ao iniciar
    wxCommandEvent dummy;
    OnLoadExample(dummy);

    // Adapta o tamanho inicial da janela à área útil do monitor (evita corte em telas 14" / 15")
    wxRect displayArea = wxGetClientDisplayRect();
    int targetW = std::min(1220, static_cast<int>(displayArea.GetWidth() * 0.92));
    int targetH = std::min(780, static_cast<int>(displayArea.GetHeight() * 0.90));
    SetMinSize(wxSize(820, 560));
    SetSize(targetW, targetH);
    Layout();
    CentreOnScreen();
}

void MainFrame::SetupGrid()
{
    const std::vector<wxString> headers = {
        wxString::FromUTF8("Tipo"),               // 0: Fase | Para-raios
        wxString::FromUTF8("Circuito"),           // 1: 1, 2 (0 para GW)
        wxString::FromUTF8("ID"),                 // 2: A, B, C, GW1...
        wxString::FromUTF8("X Torre [m]"),        // 3: x_tower
        wxString::FromUTF8("Y Torre [m]"),        // 4: y_tower
        wxString::FromUTF8("Usar Y_meio/Flecha"), // 5: Sim | Nao
        wxString::FromUTF8("Y Meio Vão [m]"),     // 6: y_midSpan
        wxString::FromUTF8("Flecha Proj [m]"),    // 7: sagProject
        wxString::FromUTF8("N Subcond."),         // 8: numSubconductors
        wxString::FromUTF8("Espaç. Feixe [m]"),   // 9: spacing
        wxString::FromUTF8("Ângulo [°]"),         // 10: angleDeg
        wxString::FromUTF8("Arranjo / Aterr."),   // 11: Single, Horizontal, Triangular, Square, Circular | Continuous, Segmented
        wxString::FromUTF8("Cabo (Catálogo)"),    // 12: codeName
        wxString::FromUTF8("Rcc 20°C [Ω/km]"),    // 13: r_dc_20
        wxString::FromUTF8("Diâm. Ext [mm]"),     // 14: outsideDiameter
        wxString::FromUTF8("GMR (Ds) [mm]"),      // 15: gmr
        wxString::FromUTF8("Peso [kg/m]"),        // 16: weightPerMeter
        wxString::FromUTF8("Alpha20 [1/°C]"),     // 17: alpha20
        wxString::FromUTF8("Skin T/D")            // 18: skinRatio
    };

    int targetCols = static_cast<int>(headers.size());
    int currentCols = grid_conductors->GetNumberCols();

    if (currentCols == 0) {
        grid_conductors->CreateGrid(0, targetCols);
    } else {
        if (grid_conductors->GetNumberRows() > 0) {
            grid_conductors->DeleteRows(0, grid_conductors->GetNumberRows());
        }
        if (currentCols < targetCols) {
            grid_conductors->AppendCols(targetCols - currentCols);
        } else if (currentCols > targetCols) {
            grid_conductors->DeleteCols(targetCols, currentCols - targetCols);
        }
    }

    for (int col = 0; col < targetCols; ++col) {
        grid_conductors->SetColLabelValue(col, headers[col]);
    }

    grid_conductors->AutoSizeColumns();
}

void MainFrame::AppendGridRow(const wxString& tipo, int circuito, const wxString& id,
                              double xTorre, double yTorre, bool temYMeio, double yMeio, double flecha,
                              int nSub, double espac, double angulo, const wxString& arranjoOuAterr,
                              const wxString& nomeCabo, double rdc20, double diamMm, double gmrMm,
                              double pesoKgM, double alpha20, double skinRatio)
{
    int row = grid_conductors->GetNumberRows();
    grid_conductors->AppendRows(1);

    wxString tipos[] = { wxString::FromUTF8("Fase"), wxString::FromUTF8("Para-raios") };
    grid_conductors->SetCellEditor(row, 0, new wxGridCellChoiceEditor(2, tipos));

    wxString simNao[] = { wxString::FromUTF8("Sim"), wxString::FromUTF8("Nao") };
    grid_conductors->SetCellEditor(row, 5, new wxGridCellChoiceEditor(2, simNao));

    wxString opcoesArranjo[] = {
        "Single", "Horizontal", "Triangular", "Square", "Circular", "Continuous", "Segmented"
    };
    grid_conductors->SetCellEditor(row, 11, new wxGridCellChoiceEditor(7, opcoesArranjo));

    grid_conductors->SetCellValue(row, 0, tipo);
    grid_conductors->SetCellValue(row, 1, wxString::Format("%d", circuito));
    grid_conductors->SetCellValue(row, 2, id);
    grid_conductors->SetCellValue(row, 3, FormatDouble(xTorre, 3));
    grid_conductors->SetCellValue(row, 4, FormatDouble(yTorre, 3));
    grid_conductors->SetCellValue(row, 5, temYMeio ? "Sim" : "Nao");
    grid_conductors->SetCellValue(row, 6, FormatDouble(yMeio, 3));
    grid_conductors->SetCellValue(row, 7, FormatDouble(flecha, 3));
    grid_conductors->SetCellValue(row, 8, wxString::Format("%d", nSub));
    grid_conductors->SetCellValue(row, 9, FormatDouble(espac, 3));
    grid_conductors->SetCellValue(row, 10, FormatDouble(angulo, 2));
    grid_conductors->SetCellValue(row, 11, arranjoOuAterr);
    grid_conductors->SetCellValue(row, 12, nomeCabo);
    grid_conductors->SetCellValue(row, 13, FormatDouble(rdc20, 4));
    grid_conductors->SetCellValue(row, 14, FormatDouble(diamMm, 2));
    grid_conductors->SetCellValue(row, 15, FormatDouble(gmrMm, 2));
    grid_conductors->SetCellValue(row, 16, FormatDouble(pesoKgM, 3));
    grid_conductors->SetCellValue(row, 17, FormatDouble(alpha20, 5));
    grid_conductors->SetCellValue(row, 18, FormatDouble(skinRatio, 2));
    
    grid_conductors->AutoSizeColumns();
}

void MainFrame::OnAddPhase(wxCommandEvent& WXUNUSED(event))
{
    int nextIdx = grid_conductors->GetNumberRows() + 1;
    AppendGridRow(wxString::FromUTF8("Fase"), 1, wxString::Format("F%d", nextIdx),
                  0.0, 20.0, true, 14.0, 0.0,
                  1, 0.45, 0.0, "Single",
                  wxString::FromUTF8("Drake (795 kcmil ACSR 26/7)"), 0.0722, 28.14, 11.37, 1.628, 0.00403, 0.5);
}

void MainFrame::OnAddGroundWire(wxCommandEvent& WXUNUSED(event))
{
    AppendGridRow(wxString::FromUTF8("Para-raios"), 0, "GW",
                  0.0, 28.0, true, 22.0, 0.0,
                  1, 0.0, 0.0, "Continuous",
                  wxString::FromUTF8("Steel 3/8 EHS"), 3.50, 9.14, 1.05, 0.407, 0.00450, 0.5);
}

void MainFrame::OnRemoveRow(wxCommandEvent& WXUNUSED(event))
{
    int row = grid_conductors->GetGridCursorRow();
    if (row >= 0 && row < grid_conductors->GetNumberRows()) {
        grid_conductors->DeleteRows(row, 1);
    }
}

void MainFrame::OnLoadExample(wxCommandEvent& WXUNUSED(event))
{
    txt_lineName->SetValue(wxString::FromUTF8("LT 230 kV Exemplo Didático (2x Drake, 2x Para-raios)"));
    txt_frequency->SetValue(FormatDouble(60.0, 1));
    txt_earthResistivity->SetValue(FormatDouble(100.0, 1));
    txt_lineLengthKm->SetValue(FormatDouble(120.0, 1));
    txt_vBaseKv->SetValue(FormatDouble(230.0, 1));
    txt_sBaseMva->SetValue(FormatDouble(100.0, 1));
    choice_transposition->SetSelection(1);

    txt_operatingTemperature->SetValue(FormatDouble(75.0, 1));
    txt_ambientTemperature->SetValue(FormatDouble(25.0, 1));
    txt_spanLength->SetValue(FormatDouble(350.0, 1));
    txt_horizontalTension->SetValue(FormatDouble(2500.0, 1));
    txt_windPressure->SetValue(FormatDouble(0.0, 1));
    txt_iceThickness->SetValue(FormatDouble(0.0, 1));

    if (grid_conductors->GetNumberRows() > 0) {
        grid_conductors->DeleteRows(0, grid_conductors->GetNumberRows());
    }

    AppendGridRow(wxString::FromUTF8("Fase"), 1, "A", -7.0, 22.0, true, 14.0, 0.0, 2, 0.45, 0.0, "Horizontal",
                  wxString::FromUTF8("Drake (795 kcmil ACSR 26/7)"), 0.0722, 28.14, 11.37, 1.628, 0.00403, 0.5);
    AppendGridRow(wxString::FromUTF8("Fase"), 1, "B",  0.0, 22.0, true, 14.0, 0.0, 2, 0.45, 0.0, "Horizontal",
                  wxString::FromUTF8("Drake (795 kcmil ACSR 26/7)"), 0.0722, 28.14, 11.37, 1.628, 0.00403, 0.5);
    AppendGridRow(wxString::FromUTF8("Fase"), 1, "C",  7.0, 22.0, true, 14.0, 0.0, 2, 0.45, 0.0, "Horizontal",
                  wxString::FromUTF8("Drake (795 kcmil ACSR 26/7)"), 0.0722, 28.14, 11.37, 1.628, 0.00403, 0.5);

    AppendGridRow(wxString::FromUTF8("Para-raios"), 0, "GW1", -4.5, 29.0, true, 23.0, 0.0, 1, 0.0, 0.0, "Continuous",
                  wxString::FromUTF8("Steel 3/8 EHS"), 3.50, 9.14, 1.05, 0.407, 0.00450, 0.5);
    AppendGridRow(wxString::FromUTF8("Para-raios"), 0, "GW2",  4.5, 29.0, true, 23.0, 0.0, 1, 0.0, 0.0, "Continuous",
                  wxString::FromUTF8("Steel 3/8 EHS"), 3.50, 9.14, 1.05, 0.407, 0.00450, 0.5);
}

wxString MainFrame::FormatDouble(double val, int decimals)
{
    // Formata usando o separador decimal do sistema (vírgula no Brasil, ponto nos EUA), sem separador de milhar
    return wxNumberFormatter::ToString(val, decimals, wxNumberFormatter::Style_None);
}

double MainFrame::ParseDouble(const wxString& str, double defaultVal)
{
    wxString s = str.Strip(wxString::both);
    if (s.IsEmpty()) return defaultVal;

    // Primeiro tenta ler respeitando o separador regional do Windows (vírgula ou ponto)
    double val = defaultVal;
    if (wxNumberFormatter::FromString(s, &val)) {
        return val;
    }

    // Fallback: se o usuário colar um valor com ponto em Windows pt-BR (ou vírgula em en-US), converte e lê
    wxString decSep = wxNumberFormatter::GetDecimalSeparator();
    if (decSep == ',') {
        s.Replace(".", ",");
    } else {
        s.Replace(",", ".");
    }
    if (wxNumberFormatter::FromString(s, &val)) {
        return val;
    }
    return defaultVal;
}

int MainFrame::ParseInt(const wxString& str, int defaultVal)
{
    long val = defaultVal;
    if (str.ToLong(&val)) return static_cast<int>(val);
    return defaultVal;
}

LineParameters::LineSystemInput MainFrame::ReadInputFromGUI()
{
    using namespace LineParameters;

    if (grid_conductors->IsCellEditControlShown()) {
        grid_conductors->SaveEditControlValue();
        grid_conductors->HideCellEditControl();
    }

    LineSystemInput line;
    line.lineName         = txt_lineName->GetValue().ToUTF8().data();
    line.frequency        = ParseDouble(txt_frequency->GetValue(), 60.0);
    line.earthResistivity = ParseDouble(txt_earthResistivity->GetValue(), 100.0);
    line.lineLengthKm     = ParseDouble(txt_lineLengthKm->GetValue(), 100.0);
    line.vBaseKv          = ParseDouble(txt_vBaseKv->GetValue(), 230.0);
    line.sBaseMva         = ParseDouble(txt_sBaseMva->GetValue(), 100.0);

    line.transposition = (choice_transposition->GetSelection() == 1)
                         ? TranspositionMode::Transposed
                         : TranspositionMode::Untransposed;

    line.environment.operatingTemperature = ParseDouble(txt_operatingTemperature->GetValue(), 75.0);
    line.environment.ambientTemperature   = ParseDouble(txt_ambientTemperature->GetValue(), 25.0);
    line.environment.spanLength           = ParseDouble(txt_spanLength->GetValue(), 300.0);
    line.environment.horizontalTension    = ParseDouble(txt_horizontalTension->GetValue(), 2000.0);
    line.environment.windPressure         = ParseDouble(txt_windPressure->GetValue(), 0.0);
    line.environment.iceThickness         = ParseDouble(txt_iceThickness->GetValue(), 0.0);

    int numRows = grid_conductors->GetNumberRows();
    for (int r = 0; r < numRows; ++r) {
        wxString tipo = grid_conductors->GetCellValue(r, 0).Trim().Lower();

        ConductorCatalog cat;
        cat.codeName        = grid_conductors->GetCellValue(r, 12).ToUTF8().data();
        cat.r_dc_20         = ParseDouble(grid_conductors->GetCellValue(r, 13), 0.0);
        cat.outsideDiameter = ParseDouble(grid_conductors->GetCellValue(r, 14), 0.0);
        cat.gmr             = ParseDouble(grid_conductors->GetCellValue(r, 15), 0.0);
        cat.weightPerMeter  = ParseDouble(grid_conductors->GetCellValue(r, 16), 0.0);
        cat.alpha20         = ParseDouble(grid_conductors->GetCellValue(r, 17), 0.00403);
        cat.skinRatio       = ParseDouble(grid_conductors->GetCellValue(r, 18), 0.5);

        bool hasMid = (grid_conductors->GetCellValue(r, 5).Trim().Lower() == "sim");
        double yMid = ParseDouble(grid_conductors->GetCellValue(r, 6), 0.0);
        double sagP = ParseDouble(grid_conductors->GetCellValue(r, 7), 0.0);

        if (tipo.StartsWith("para")) {
            GroundWireData gw;
            gw.wireId           = grid_conductors->GetCellValue(r, 2).ToUTF8().data();
            gw.x_tower          = ParseDouble(grid_conductors->GetCellValue(r, 3), 0.0);
            gw.y_tower          = ParseDouble(grid_conductors->GetCellValue(r, 4), 0.0);
            gw.hasMidSpanHeight = hasMid;
            gw.y_midSpan        = yMid;
            gw.sagProject       = sagP;

            wxString aterr = grid_conductors->GetCellValue(r, 11).Trim().Lower();
            gw.grounding = (aterr == "segmented")
                           ? GroundWireGrounding::Segmented
                           : GroundWireGrounding::Continuous;
            gw.conductor = cat;
            line.groundWires.push_back(gw);
        } else {
            PhaseData ph;
            ph.circuitId        = ParseInt(grid_conductors->GetCellValue(r, 1), 1);
            ph.phaseId          = grid_conductors->GetCellValue(r, 2).ToUTF8().data();
            ph.x_tower          = ParseDouble(grid_conductors->GetCellValue(r, 3), 0.0);
            ph.y_tower          = ParseDouble(grid_conductors->GetCellValue(r, 4), 0.0);
            ph.hasMidSpanHeight = hasMid;
            ph.y_midSpan        = yMid;
            ph.sagProject       = sagP;

            ph.bundle.numSubconductors = ParseInt(grid_conductors->GetCellValue(r, 8), 1);
            ph.bundle.spacing          = ParseDouble(grid_conductors->GetCellValue(r, 9), 0.45);
            ph.bundle.angleDeg         = ParseDouble(grid_conductors->GetCellValue(r, 10), 0.0);

            wxString arr = grid_conductors->GetCellValue(r, 11).Trim().Lower();
            if (arr == "horizontal")      ph.bundle.arrangement = BundleArrangement::Horizontal;
            else if (arr == "triangular") ph.bundle.arrangement = BundleArrangement::Triangular;
            else if (arr == "square")     ph.bundle.arrangement = BundleArrangement::Square;
            else if (arr == "circular")   ph.bundle.arrangement = BundleArrangement::Circular;
            else                          ph.bundle.arrangement = BundleArrangement::Single;

            ph.conductor = cat;
            line.phases.push_back(ph);
        }
    }

    return line;
}

void MainFrame::OnCalculate(wxCommandEvent& WXUNUSED(event))
{
    try {
        LineParameters::LineSystemInput line = ReadInputFromGUI();
        if (line.phases.empty()) {
            wxMessageBox(wxString::FromUTF8("Adicione pelo menos uma fase na planilha antes de calcular."),
                         wxString::FromUTF8("Aviso"), wxOK | wxICON_WARNING, this);
            return;
        }

        auto matrices = LineParameters::LineParametersEngine::compute(line);
        auto seq = LineParameters::SymmetricalComponentsEngine::analyze(
            matrices.Z_phase, matrices.C_phase, matrices.Y_phase, line
        );

        std::string report = LineParameters::ReportGenerator::generateReport(line, matrices, seq);
        wxString reportWx = wxString::FromUTF8(report.c_str());
        if (wxNumberFormatter::GetDecimalSeparator() == ',') {
            reportWx.Replace(".", ",");
        }
        txt_reportOutput->SetValue(reportWx);

    } catch (const std::exception& ex) {
        wxMessageBox(wxString::Format(wxString::FromUTF8("Erro durante o cálculo: %s"), ex.what()),
                     wxString::FromUTF8("Erro de Execução"), wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnExportReport(wxCommandEvent& WXUNUSED(event))
{
    wxString content = txt_reportOutput->GetValue();
    if (content.IsEmpty()) {
        wxMessageBox(wxString::FromUTF8("Execute o cálculo primeiro para gerar um relatório."),
                     wxString::FromUTF8("Aviso"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog saveDlg(this, wxString::FromUTF8("Salvar Relatório de Parâmetros"), "", "relatorio_lt.txt",
                         "Arquivos de Texto (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDlg.ShowModal() == wxID_CANCEL) return;

    wxFileOutputStream output(saveDlg.GetPath());
    if (!output.IsOk()) {
        wxMessageBox(wxString::FromUTF8("Não foi possível gravar o arquivo selecionado."),
                     wxString::FromUTF8("Erro"), wxOK | wxICON_ERROR, this);
        return;
    }
    wxTextOutputStream textOut(output, wxEOL_NATIVE, wxConvUTF8);
    textOut.WriteString(content);
}

void MainFrame::OnClearReport(wxCommandEvent& WXUNUSED(event))
{
    txt_reportOutput->Clear();
}

void MainFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo info;
    info.SetName(wxString::FromUTF8("Calculadora de Parâmetros Elétricos de Linhas de Transmissão"));
    info.SetVersion("1.0.0");
    info.SetDescription(wxString::FromUTF8(
        "Ferramenta para cálculo de matrizes de impedância longitudinal\n"
        "(equações de Carson), capacitância, admitância transversal,\n"
        "redução de Kron para cabos para-raios e decomposição em\n"
        "componentes simétricas (sequências zero, positiva e negativa)\n"
        "de linhas de transmissão aéreas.\n\n"
        "Desenvolvido em C++17 com interface gráfica nativa baseada em\n"
        "wxWidgets 3.2 (compilação estática via MSYS2 / MinGW-w64 UCRT64)."
    ));
    info.SetCopyright(wxString::FromUTF8("© 2026 Luciano Meurer — Todos os direitos reservados."));
    info.AddDeveloper(wxString::FromUTF8("Luciano Meurer (Engenheiro Eletricista)"));
    info.SetWebSite("https://github.com/lmeurer74/calculo-impedancia-lt", wxString::FromUTF8("Repositório do Projeto no GitHub"));

    wxAboutBox(info, this);
}

void MainFrame::ApplyConductorToRow(int row, const LineParameters::ConductorCatalog& cond)
{
    grid_conductors->SetCellValue(row, 12, wxString::FromUTF8(cond.codeName.c_str()));
    grid_conductors->SetCellValue(row, 13, FormatDouble(cond.r_dc_20, 4));
    grid_conductors->SetCellValue(row, 14, FormatDouble(cond.outsideDiameter, 2));
    grid_conductors->SetCellValue(row, 15, FormatDouble(cond.gmr, 2));
    grid_conductors->SetCellValue(row, 16, FormatDouble(cond.weightPerMeter, 3));
    grid_conductors->SetCellValue(row, 17, FormatDouble(cond.alpha20, 5));
    grid_conductors->SetCellValue(row, 18, FormatDouble(cond.skinRatio, 2));
}

void MainFrame::OnGridCellDoubleClick(wxGridEvent& event)
{
    if (event.GetCol() != 12) {
        event.Skip(); // Permite edição normal nas demais colunas
        return;
    }

    int row = event.GetRow();
    wxString currentCable = grid_conductors->GetCellValue(row, 12);

    std::string catalogPath;
    bool foundCatalog = ConductorCatalogManager::findCatalogFile(catalogPath);

    if (!foundCatalog) {
        int resp = wxMessageBox(
            wxString::FromUTF8(
                "O arquivo 'cabos_catalogo.csv' não foi encontrado na pasta do executável "
                "(nem na subpasta 'data'), portanto o acervo de condutores não está disponível.\n\n"
                "Você pode preencher os parâmetros manualmente direto na planilha.\n"
                "Deseja abrir a tela para incluir um novo condutor agora?"
            ),
            wxString::FromUTF8("Catálogo de Condutores Não Encontrado"),
            wxYES_NO | wxICON_INFORMATION,
            this
        );

        if (resp != wxYES) {
            return;
        }
        currentCable.Clear();
    }

    // Passa catalogPath explicitamente para o diálogo carregar o arquivo
    ConductorPickerDialog dlg(this, currentCable, catalogPath);
    if (dlg.ShowModal() == wxID_OK) {
        auto cond = dlg.GetSelectedConductor();
        ApplyConductorToRow(row, cond);

        // Verifica se há outras linhas com o mesmo Tipo e mesmo Circuito para replicar
        wxString tipoAtual = grid_conductors->GetCellValue(row, 0).Trim();
        wxString circAtual = grid_conductors->GetCellValue(row, 1).Trim();
        std::vector<int> matchingRows;

        for (int r = 0; r < grid_conductors->GetNumberRows(); ++r) {
            if (r == row) continue;
            if (grid_conductors->GetCellValue(r, 0).Trim() == tipoAtual &&
                grid_conductors->GetCellValue(r, 1).Trim() == circAtual) {
                matchingRows.push_back(r);
            }
        }

        if (!matchingRows.empty()) {
            wxString msg = wxString::Format(
                wxString::FromUTF8("Deseja replicar este condutor para as outras %zu linha(s) de '%s' (Circuito %s)?"),
                matchingRows.size(), tipoAtual, circAtual
            );
            if (wxMessageBox(msg, wxString::FromUTF8("Replicar Condutor"), wxYES_NO | wxICON_QUESTION, this) == wxYES) {
                for (int targetRow : matchingRows) {
                    ApplyConductorToRow(targetRow, cond);
                }
            }
        }
        grid_conductors->AutoSizeColumns();
    }
}