/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Auto-test du moteur de cohérence (chantier B1), sans réseau : chaque cas fixe
 * les trois vues (source o2switch / collecteur / cache) et l'état attendu pour un
 * fichier, puis vérifie que CoherenceEngine::evaluate le retrouve. Sert de banc de
 * démonstration lisible avant tout câblage d'interface.
 *
 *   sitewatch-coherence-selftest        # tableau + bilan, code retour 0/1
 */

#include "coherence/CoherenceEngine.h"

#include <iostream>
#include <string>
#include <vector>

using namespace coherence;

namespace {

struct Case {
    std::string title;      // description du scénario
    PathView    source;
    PathView    collector;
    PathView    cache;
    std::string file;       // fichier examiné
    FileState   expected;   // état attendu pour ce fichier
};

std::string sz(int64_t v, bool known) {
    if (!known) return "  —  ";      // absent / injoignable (voir *Known)
    return std::to_string(v);
}

const FileRow* rowFor(const std::vector<FileRow>& rows, const std::string& name) {
    for (const auto& r : rows)
        if (r.name == name) return &r;
    return nullptr;
}

} // namespace

int main() {
    // Un mois « en cours » : monsite.fr-Aug-2026.gz. La source fait 4096 o.
    const std::string F = "monsite.fr-Aug-2026.gz";

    std::vector<Case> cases = {
        { "Tout aligné (cache == collecteur == source)",
          {true, {{F, 4096}}}, {true, {{F, 4096}}}, {true, {{F, 4096}}}, F, FileState::UpToDate },

        { "Cache en retard (le mois a grossi, cache pas resynchronisé)",
          {true, {{F, 4096}}}, {true, {{F, 4096}}}, {true, {{F, 2048}}}, F, FileState::CacheBehind },

        { "Collecteur en retard (cache à jour en direct, collecteur pas encore)",
          {true, {{F, 4096}}}, {true, {{F, 2048}}}, {true, {{F, 4096}}}, F, FileState::CollectorBehind },

        { "Absent du cache (jamais téléchargé localement)",
          {true, {{F, 4096}}}, {true, {{F, 4096}}}, {true, {}},          F, FileState::MissingFromCache },

        { "Absent du collecteur (pas encore collecté)",
          {true, {{F, 4096}}}, {true, {}},          {true, {{F, 4096}}}, F, FileState::MissingFromCollector },

        { "Source o2switch injoignable (contrôle partiel)",
          {false, {}}, {true, {{F, 4096}}}, {true, {{F, 2048}}}, F, FileState::SourceUnreachable },

        { "Collecteur injoignable (cache bon vs source)",
          {true, {{F, 4096}}}, {false, {}}, {true, {{F, 4096}}}, F, FileState::CollectorUnreachable },

        { "Divergence inexpliquée (cache plus gros que la source)",
          {true, {{F, 4096}}}, {true, {{F, 4096}}}, {true, {{F, 8192}}}, F, FileState::UnexplainedDivergence },

        { "Archivé : mois absent de la source, présent partout ailleurs (sain)",
          {true, {}}, {true, {{F, 4096}}}, {true, {{F, 4096}}}, F, FileState::UpToDate },

        { "Archivé mais pas dans le cache : remplissable depuis le collecteur",
          {true, {}}, {true, {{F, 4096}}}, {true, {}}, F, FileState::MissingFromCache },
    };

    std::cout << "Auto-test du moteur de cohérence SiteWatch (B1)\n"
              << "-----------------------------------------------\n";
    std::cout << "  Fichier  |  source | collec. |  cache  | état obtenu\n";

    int failures = 0;
    for (const auto& c : cases) {
        const auto rows = evaluate(c.source, c.collector, c.cache);
        const FileRow* r = rowFor(rows, c.file);
        const bool ok = r && r->state == c.expected;
        if (!ok) ++failures;

        std::cout << (ok ? "  [ok] " : "  [KO] ");
        if (r) {
            std::cout << sz(r->sourceSize, r->sourceKnown) << " | "
                      << sz(r->collectorSize, r->collectorKnown) << " | "
                      << sz(r->cacheSize, r->cacheKnown) << " | "
                      << toString(r->state);
            if (!r->note.empty()) std::cout << "  (" << r->note << ")";
        } else {
            std::cout << "aucune ligne pour " << c.file;
        }
        std::cout << "\n         " << c.title << "\n";
        if (!ok && r)
            std::cout << "         ATTENDU : " << toString(c.expected) << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    if (failures == 0)
        std::cout << "Tous les cas passent (" << cases.size() << ").\n";
    else
        std::cout << failures << " cas en échec sur " << cases.size() << ".\n";
    return failures == 0 ? 0 : 1;
}
