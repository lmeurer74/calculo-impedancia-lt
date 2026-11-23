#include <wx/wx.h>
#include "MainFrame.h"

class LineCalcApp : public wxApp
{
private:
    wxLocale m_locale;

public:
    virtual bool OnInit() override
    {
        if (!wxApp::OnInit()) return false;

        // Adota automaticamente o idioma e separador decimal do Windows (ex.: pt_BR -> vírgula, en_US -> ponto)
        m_locale.Init(wxLANGUAGE_DEFAULT);
        
        MainFrame* frame = new MainFrame(nullptr);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(LineCalcApp);