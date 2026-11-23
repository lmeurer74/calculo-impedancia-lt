#include "ConductorPickerDialog.hpp"
#include <wx/msgdlg.h>
#include <wx/numformatter.h>
#include <set>

static wxString FmtReg(double val, int dec) {
    return wxNumberFormatter::ToString(val, dec, wxNumberFormatter::Style_None);
}

static double ParseReg(const wxString& str, double defVal = 0.0) {
    wxString s = str.Strip(wxString::both);
    if (s.IsEmpty()) return defVal;
    double v = defVal;
    if (wxNumberFormatter::FromString(s, &v)) return v;
    if (wxNumberFormatter::GetDecimalSeparator() == ',') s.Replace(".", ",");
    else s.Replace(",", ".");
    if (wxNumberFormatter::FromString(s, &v)) return v;
    return defVal;
}

ConductorPickerDialog::ConductorPickerDialog(wxWindow* parent,
                                             const wxString& currentCableName,
                                             const std::string& catalogFilePath)
    : ConductorPickerDialogBase(parent),
      m_catalogFilePath(catalogFilePath)
{
    if (!m_catalogFilePath.empty()) {
        m_allEntries = ConductorCatalogManager::loadFromCsv(m_catalogFilePath);
    }

    PopulateTipoFilter();
    RefreshList();

    // Tenta pré-selecionar o cabo atual da célula
    if (!currentCableName.IsEmpty()) {
        for (size_t i = 0; i < m_filteredIndices.size(); ++i) {
            if (wxString::FromUTF8(m_allEntries[m_filteredIndices[i]].spec.codeName.c_str()) == currentCableName) {
                m_listCables->SetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                           wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                m_listCables->EnsureVisible(i);
                LoadEntryToFields(m_allEntries[m_filteredIndices[i]]);
                break;
            }
        }
    }

    m_choiceTipo->Bind(wxEVT_CHOICE, &ConductorPickerDialog::OnFilterChanged, this);
    m_txtSearch->Bind(wxEVT_TEXT, &ConductorPickerDialog::OnFilterChanged, this);
    m_listCables->Bind(wxEVT_LIST_ITEM_SELECTED, &ConductorPickerDialog::OnItemSelected, this);
    m_listCables->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ConductorPickerDialog::OnItemActivated, this);
    m_btnSaveCsv->Bind(wxEVT_BUTTON, &ConductorPickerDialog::OnSaveToCatalog, this);
    Bind(wxEVT_BUTTON, &ConductorPickerDialog::OnConfirmOK, this, wxID_OK);
}

void ConductorPickerDialog::PopulateTipoFilter() {
    m_choiceTipo->Clear();
    m_choiceTipo->Append("Todos");
    std::set<std::string> tipos;
    for (const auto& e : m_allEntries) {
        if (!e.tipo.empty()) tipos.insert(e.tipo);
    }
    for (const auto& t : tipos) {
        m_choiceTipo->Append(wxString::FromUTF8(t.c_str()));
    }
    m_choiceTipo->SetSelection(0);
}

void ConductorPickerDialog::RefreshList() {
    m_listCables->DeleteAllItems();
    m_filteredIndices.clear();

    wxString selTipo = (m_choiceTipo->GetSelection() > 0) ? m_choiceTipo->GetStringSelection() : "";
    wxString query = m_txtSearch->GetValue().Trim().Lower();

    for (size_t idx = 0; idx < m_allEntries.size(); ++idx) {
        const auto& e = m_allEntries[idx];
        wxString wTipo = wxString::FromUTF8(e.tipo.c_str());
        wxString wNome = wxString::FromUTF8(e.spec.codeName.c_str());

        if (!selTipo.IsEmpty() && wTipo != selTipo) continue;
        if (!query.IsEmpty() && !wNome.Lower().Contains(query) &&
            !wxString::FromUTF8(e.formacao.c_str()).Lower().Contains(query)) {
            continue;
        }

        long itemIdx = m_listCables->InsertItem(m_listCables->GetItemCount(), wTipo);
        m_listCables->SetItem(itemIdx, 1, wxString::FromUTF8(e.serie.c_str()));
        m_listCables->SetItem(itemIdx, 2, wNome);
        m_listCables->SetItem(itemIdx, 3, wxString::FromUTF8(e.formacao.c_str()));
        m_listCables->SetItem(itemIdx, 4, FmtReg(e.spec.outsideDiameter, 2));
        m_listCables->SetItem(itemIdx, 5, FmtReg(e.spec.r_dc_20, 4));
        m_listCables->SetItem(itemIdx, 6, FmtReg(e.spec.gmr, 2));

        m_filteredIndices.push_back(idx);
    }
}

void ConductorPickerDialog::LoadEntryToFields(const CableCatalogEntry& entry) {
    m_txtName->SetValue(wxString::FromUTF8(entry.spec.codeName.c_str()));
    m_txtTipoManual->SetValue(wxString::FromUTF8(entry.tipo.c_str()));
    m_txtRdc20->SetValue(FmtReg(entry.spec.r_dc_20, 4));
    m_txtDiam->SetValue(FmtReg(entry.spec.outsideDiameter, 2));
    m_txtGmr->SetValue(FmtReg(entry.spec.gmr, 2));
    m_txtPeso->SetValue(FmtReg(entry.spec.weightPerMeter, 3));
    m_txtAlpha20->SetValue(FmtReg(entry.spec.alpha20, 5));
    m_txtSkin->SetValue(FmtReg(entry.spec.skinRatio, 2));
}

void ConductorPickerDialog::OnFilterChanged(wxCommandEvent& WXUNUSED(event)) {
    RefreshList();
}

void ConductorPickerDialog::OnItemSelected(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx >= 0 && static_cast<size_t>(idx) < m_filteredIndices.size()) {
        LoadEntryToFields(m_allEntries[m_filteredIndices[idx]]);
    }
}

void ConductorPickerDialog::OnItemActivated(wxListEvent& event) {
    OnItemSelected(event);
    wxCommandEvent dummy;
    OnConfirmOK(dummy);
}

void ConductorPickerDialog::OnSaveToCatalog(wxCommandEvent& WXUNUSED(event)) {
    CableCatalogEntry e;
    e.tipo                 = m_txtTipoManual->GetValue().Trim().ToUTF8().data();
    e.serie                = "Custom";
    e.formacao             = "-";
    e.spec.codeName        = m_txtName->GetValue().Trim().ToUTF8().data();
    e.spec.r_dc_20         = ParseReg(m_txtRdc20->GetValue(), 0.0);
    e.spec.outsideDiameter = ParseReg(m_txtDiam->GetValue(), 0.0);
    e.spec.gmr             = ParseReg(m_txtGmr->GetValue(), 0.0);
    e.spec.weightPerMeter  = ParseReg(m_txtPeso->GetValue(), 0.0);
    e.spec.alpha20         = ParseReg(m_txtAlpha20->GetValue(), 0.00403);
    e.spec.skinRatio       = ParseReg(m_txtSkin->GetValue(), 0.50);

    if (e.spec.codeName.empty()) {
        wxMessageBox(wxString::FromUTF8("Informe o Nome/Código do condutor antes de salvar."),
                     wxString::FromUTF8("Aviso"), wxOK | wxICON_WARNING, this);
        m_txtName->SetFocus();
        return;
    }

    // Verifica se já existe um condutor com este mesmo nome no catálogo
    bool alreadyExists = false;
    wxString targetName = wxString::FromUTF8(e.spec.codeName.c_str()).Trim().Lower();
    for (const auto& item : m_allEntries) {
        if (wxString::FromUTF8(item.spec.codeName.c_str()).Trim().Lower() == targetName) {
            alreadyExists = true;
            break;
        }
    }

    if (alreadyExists) {
        int resp = wxMessageBox(
            wxString::FromUTF8("Já existe um condutor cadastrado com este mesmo nome no catálogo.\n\n"
                               "Deseja atualizar as informações do condutor existente?\n"
                               "(Clique em 'Não' para cancelar e cadastrar com um novo nome.)"),
            wxString::FromUTF8("Condutor Já Existente"),
            wxYES_NO | wxICON_QUESTION,
            this
        );

        if (resp != wxYES) {
            wxMessageBox(
                wxString::FromUTF8("Operação cancelada. Por favor, altere o campo 'Nome / Código' "
                                   "para manter o original e salvar como um novo condutor."),
                wxString::FromUTF8("Operação Cancelada"),
                wxOK | wxICON_INFORMATION,
                this
            );
            m_txtName->SetFocus();
            m_txtName->SelectAll();
            return;
        }
    }

    if (ConductorCatalogManager::saveOrUpdateCsv(m_allEntries, e, alreadyExists, m_catalogFilePath)) {
        if (m_catalogFilePath.empty()) {
            ConductorCatalogManager::findCatalogFile(m_catalogFilePath);
        }
        PopulateTipoFilter();
        RefreshList();
        wxString msg = alreadyExists
            ? wxString::FromUTF8("Informações do condutor atualizadas com sucesso no arquivo cabos_catalogo.csv!")
            : wxString::FromUTF8("Condutor salvo com sucesso em cabos_catalogo.csv (na pasta do executável)!");
        wxMessageBox(msg, wxString::FromUTF8("Catálogo Atualizado"), wxOK | wxICON_INFORMATION, this);
    }
}

void ConductorPickerDialog::OnConfirmOK(wxCommandEvent& WXUNUSED(event)) {
    m_selected.codeName        = m_txtName->GetValue().Trim().ToUTF8().data();
    m_selected.r_dc_20         = ParseReg(m_txtRdc20->GetValue(), 0.0);
    m_selected.outsideDiameter = ParseReg(m_txtDiam->GetValue(), 0.0);
    m_selected.gmr             = ParseReg(m_txtGmr->GetValue(), 0.0);
    m_selected.weightPerMeter  = ParseReg(m_txtPeso->GetValue(), 0.0);
    m_selected.alpha20         = ParseReg(m_txtAlpha20->GetValue(), 0.00403);
    m_selected.skinRatio       = ParseReg(m_txtSkin->GetValue(), 0.50);

    if (m_selected.codeName.empty()) {
        wxMessageBox(wxString::FromUTF8("Selecione um condutor na lista ou preencha o nome manualmente."),
                     wxString::FromUTF8("Aviso"), wxOK | wxICON_WARNING, this);
        return;
    }
    EndModal(wxID_OK);
}