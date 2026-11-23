#include "ConductorCatalogManager.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

static fs::path getExeDir() {
    wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
    return fs::path(exePath.GetPath().ToStdWstring());
}

static fs::path toFsPath(const std::string& utf8Path) {
    return fs::path(wxString::FromUTF8(utf8Path.c_str()).ToStdWstring());
}

static std::string fromFsPath(const fs::path& p) {
    return wxString(p.wstring()).ToUTF8().data();
}

std::string ConductorCatalogManager::getDefaultSavePath() {
    fs::path p = getExeDir() / L"cabos_catalogo.csv";
    return fromFsPath(p);
}

bool ConductorCatalogManager::findCatalogFile(std::string& outFullPath) {
    std::error_code ec;
    fs::path exeDir = getExeDir();

    // 1. Procura primeiro na mesma pasta do executável
    fs::path pRoot = exeDir / L"cabos_catalogo.csv";
    if (fs::exists(pRoot, ec) && fs::is_regular_file(pRoot, ec)) {
        outFullPath = fromFsPath(pRoot);
        return true;
    }

    // 2. Fallback: verifica subpasta data/ ao lado do executável
    fs::path pData = exeDir / L"data" / L"cabos_catalogo.csv";
    if (fs::exists(pData, ec) && fs::is_regular_file(pData, ec)) {
        outFullPath = fromFsPath(pData);
        return true;
    }

    // 3. Fallback: verifica diretório de trabalho atual (útil ao rodar via terminal)
    fs::path pCwd = fs::current_path(ec) / L"cabos_catalogo.csv";
    if (!ec && fs::exists(pCwd, ec) && fs::is_regular_file(pCwd, ec)) {
        outFullPath = fromFsPath(pCwd);
        return true;
    }

    outFullPath.clear();
    return false;
}

static std::string trimStr(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n\"");
    return s.substr(start, end - start + 1);
}

static double parseNumFlexible(std::string s, double defVal = 0.0) {
    s = trimStr(s);
    if (s.empty() || s == "-") return defVal;
    std::replace(s.begin(), s.end(), ',', '.');
    wxString ws = wxString::FromUTF8(s.c_str());
    double val = defVal;
    if (ws.ToCDouble(&val)) {
        return val;
    }
    return defVal;
}

std::vector<CableCatalogEntry> ConductorCatalogManager::loadFromCsv(const std::string& fullPath) {
    std::vector<CableCatalogEntry> entries;
    if (fullPath.empty()) return entries;

    std::ifstream in(toFsPath(fullPath));
    if (!in.is_open()) return entries;

    std::string line;
    bool firstLine = true;
    while (std::getline(in, line)) {
        if (firstLine) {
            firstLine = false;
            if (line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
                line.erase(0, 3);
            }
            if (line.find("Nome") != std::string::npos) continue;
        }
        if (trimStr(line).empty()) continue;

        char delim = (line.find(';') != std::string::npos) ? ';' : ',';
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> cols;
        while (std::getline(ss, token, delim)) {
            cols.push_back(trimStr(token));
        }

        if (cols.size() < 9) continue;

        CableCatalogEntry e;
        e.tipo                  = cols[0];
        e.serie                 = cols[1];
        e.spec.codeName         = cols[2];
        e.secaoMm2              = parseNumFlexible(cols[3], 0.0);
        e.formacao              = cols[4];
        e.spec.outsideDiameter  = parseNumFlexible(cols[5], 0.0);
        e.spec.weightPerMeter   = parseNumFlexible(cols[6], 0.0);
        e.spec.r_dc_20          = parseNumFlexible(cols[7], 0.0);
        e.spec.gmr              = parseNumFlexible(cols[8], 0.0);
        e.spec.alpha20          = (cols.size() > 9)  ? parseNumFlexible(cols[9],  0.00403) : 0.00403;
        e.spec.skinRatio        = (cols.size() > 10) ? parseNumFlexible(cols[10], 0.50)    : 0.50;

        entries.push_back(e);
    }
    return entries;
}

bool ConductorCatalogManager::saveOrUpdateCsv(std::vector<CableCatalogEntry>& entries,
                                              const CableCatalogEntry& entry,
                                              bool updateExisting,
                                              const std::string& fullPath)
{
    std::string targetPath = fullPath;
    if (targetPath.empty()) {
        if (!findCatalogFile(targetPath)) {
            targetPath = getDefaultSavePath();
        }
    }

    bool found = false;
    wxString targetName = wxString::FromUTF8(entry.spec.codeName.c_str()).Trim().Lower();

    if (updateExisting) {
        for (auto& existing : entries) {
            wxString curName = wxString::FromUTF8(existing.spec.codeName.c_str()).Trim().Lower();
            if (curName == targetName) {
                CableCatalogEntry merged = entry;
                if (merged.serie == "Custom" && !existing.serie.empty()) merged.serie = existing.serie;
                if (merged.formacao == "-" && !existing.formacao.empty()) merged.formacao = existing.formacao;
                if (merged.secaoMm2 <= 0.0) merged.secaoMm2 = existing.secaoMm2;
                existing = merged;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        entries.push_back(entry);
    }

    std::ofstream out(toFsPath(targetPath), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    out << "\xEF\xBB\xBF";
    out << "Tipo;Serie;Nome;Secao_mm2;Formacao;Diametro_mm;Peso_kg_m;Rcc_20C_ohm_km;GMR_mm;Alpha20_1_C;Skin_TD\n";

    for (const auto& e : entries) {
        out << e.tipo << ";"
            << e.serie << ";"
            << e.spec.codeName << ";"
            << wxString::FromCDouble(e.secaoMm2, 2).ToStdString() << ";"
            << e.formacao << ";"
            << wxString::FromCDouble(e.spec.outsideDiameter, 2).ToStdString() << ";"
            << wxString::FromCDouble(e.spec.weightPerMeter, 4).ToStdString() << ";"
            << wxString::FromCDouble(e.spec.r_dc_20, 5).ToStdString() << ";"
            << wxString::FromCDouble(e.spec.gmr, 2).ToStdString() << ";"
            << wxString::FromCDouble(e.spec.alpha20, 5).ToStdString() << ";"
            << wxString::FromCDouble(e.spec.skinRatio, 2).ToStdString() << "\n";
    }
    return true;
}