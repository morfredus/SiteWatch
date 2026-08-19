/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// CoherenceEngine : cœur de décision du contrôle de cohérence (chantier B1).
//
// Confronte, pour un même site, les trois états possibles d'un fichier de log :
//   1. le fichier source chez o2switch ;
//   2. l'objet conservé par morfCollector (le collecteur SÉLECTIONNÉ) ;
//   3. le fichier présent dans le cache local de SiteWatch.
//
// Cette unité est PURE : elle ne fait aucune E/S. Elle reçoit trois « vues » déjà
// collectées (nom -> taille, + joignabilité) et rend un état par fichier. C'est
// ce qui la rend testable sans réseau (voir coherence_selftest).
//
// Principe (voir CONTRAT.md morfCollector §5.6) : les logs o2switch sont des .gz
// mensuels APPEND-ONLY. La seule métadonnée à sémantique comparable dans les trois
// chemins est la TAILLE ; les horodatages y ont trois sens différents (mtime
// source non captée / date de collecte / date d'écriture locale) et ne servent
// jamais de test d'égalité. Le hash imposerait un transfert : exclu.
//
// Règles cardinales :
//   - o2switch reste la source PRIMAIRE (référence quand elle est joignable) ;
//   - le cache SiteWatch reste l'autorité FONCTIONNELLE (ce qui est analysé) ;
//   - une source injoignable n'est JAMAIS traduite en « absent » ni « périmé » :
//     c'est une impossibilité de vérifier, distincte d'une divergence réelle.
// -----------------------------------------------------------------------------
namespace coherence {

// Vue d'un des trois chemins, en métadonnées seulement. `reachable` distingue
// « interrogé, voici les fichiers » de « pas pu interroger » (chemin injoignable).
// Le cache local est toujours joignable ; source et collecteur peuvent ne pas
// l'être.
struct PathView {
    bool reachable = false;
    std::map<std::string, int64_t> sizes;   // fichiers présents : nom -> taille (octets)
};

enum class FileState {
    UpToDate,               // à jour (cache == source ; ou archivé au-delà de la source)
    CollectorBehind,        // collecteur en retard (taille collecteur < source)
    CacheBehind,            // cache SiteWatch en retard (taille cache < source)
    MissingFromCollector,   // absent du collecteur
    MissingFromCache,       // absent du cache
    SourceUnreachable,      // source o2switch inaccessible (vérif vs source impossible)
    CollectorUnreachable,   // collecteur inaccessible
    UnexplainedDivergence,  // divergence inexpliquée (copie plus grosse que la source…)
};

const char* toString(FileState s);   // libellé français
bool        isActionable(FileState s); // true si une action utilisateur est proposable

struct FileRow {
    std::string name;
    int64_t sourceSize    = -1;      // -1 si absent OU chemin injoignable (voir *Known)
    int64_t collectorSize = -1;
    int64_t cacheSize     = -1;
    bool    sourceKnown    = false;  // présent sur o2switch (et source joignable)
    bool    collectorKnown = false;  // présent dans le collecteur (et collecteur joignable)
    bool    cacheKnown     = false;  // présent dans le cache local
    FileState   state = FileState::UpToDate;
    std::string note;                // précision facultative (contexte, pas un état)
};

// Évalue la cohérence des trois vues d'un même site. Critère : la TAILLE.
std::vector<FileRow> evaluate(const PathView& source,
                              const PathView& collector,
                              const PathView& cache);

} // namespace coherence
