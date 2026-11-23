#ifndef CONDUCTOR_PICKER_DIALOG_HPP
#define CONDUCTOR_PICKER_DIALOG_HPP

#include <string>
#include <vector>
#include "ConductorCatalogManager.hpp"
#include "ConductorPickerDialogBase.h" // Arquivo gerado pelo wxUiEditor


class ConductorPickerDialog : public ConductorPickerDialogBase {
public:
    ConductorPickerDialog(wxWindow* parent,
                          const wxString& currentCableName = "",
                          const std::string& catalogFilePath = "");
    LineParameters::ConductorCatalog GetSelectedConductor() const { return m_selected; }

private:
    std::string m_catalogFilePath;
    std::vector<CableCatalogEntry> m_allEntries;
    std::vector<size_t> m_filteredIndices;
    LineParameters::ConductorCatalog m_selected;

    // Nota: m_choiceTipo, m_txtSearch, m_listCables, m_txtName, etc.
    // já vêm declarados como protected em ConductorPickerDialogBase!

    void PopulateTipoFilter();
    void RefreshList();
    void LoadEntryToFields(const CableCatalogEntry& entry);

    void OnFilterChanged(wxCommandEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnSaveToCatalog(wxCommandEvent& event);
    void OnConfirmOK(wxCommandEvent& event);
};

#endif