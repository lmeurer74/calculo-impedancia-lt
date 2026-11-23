#ifndef CONDUCTOR_CATALOG_MANAGER_HPP
#define CONDUCTOR_CATALOG_MANAGER_HPP

#include <string>
#include <vector>
#include "InputData.hpp"

struct CableCatalogEntry {
    std::string tipo;      // Ex.: "CA", "CAA", "CAA-HS", "CAL", "EHS", "CC"
    std::string serie;     // Ex.: "AWG", "KCMIL", "6201", "Aco"
    std::string formacao;  // Ex.: "26/7", "7"
    double secaoMm2 = 0.0;
    LineParameters::ConductorCatalog spec; // codeName, r_dc_20, outsideDiameter, gmr, weightPerMeter, alpha20, skinRatio
};

class ConductorCatalogManager {
public:
    // Procura cabos_catalogo.csv na pasta do .exe (e fallback em subpasta data/).
    // Retorna true se encontrou o arquivo; false caso não exista.
    static bool findCatalogFile(std::string& outFullPath);

    // Retorna o caminho padrão de criação/gravação (sempre na mesma pasta do executável).
    static std::string getDefaultSavePath();

    // Carrega os condutores de um arquivo CSV existente.
    static std::vector<CableCatalogEntry> loadFromCsv(const std::string& fullPath);

    // Salva um novo condutor ou atualiza um existente no CSV (cria o arquivo se não existir).
    static bool saveOrUpdateCsv(std::vector<CableCatalogEntry>& entries,
                                const CableCatalogEntry& entry,
                                bool updateExisting,
                                const std::string& fullPath = "");
};

#endif // CONDUCTOR_CATALOG_MANAGER_HPP