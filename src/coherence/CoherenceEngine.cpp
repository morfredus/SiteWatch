/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "coherence/CoherenceEngine.h"

#include <set>

namespace coherence {

const char* toString(FileState s) {
    switch (s) {
        case FileState::UpToDate:              return "à jour";
        case FileState::CollectorBehind:       return "collecteur en retard";
        case FileState::CacheBehind:           return "cache SiteWatch en retard";
        case FileState::MissingFromCollector:  return "absent du collecteur";
        case FileState::MissingFromCache:      return "absent du cache";
        case FileState::SourceUnreachable:     return "source o2switch inaccessible";
        case FileState::CollectorUnreachable:  return "collecteur inaccessible";
        case FileState::UnexplainedDivergence: return "divergence inexpliquée";
    }
    return "état inconnu";
}

bool isActionable(FileState s) {
    // Ce qui appelle une action déclenchée par l'utilisateur (jamais automatique).
    // Les états d'INJOIGNABILITÉ ne sont pas « actionnables » ici : ils appellent
    // une nouvelle tentative, pas une resynchronisation aveugle.
    switch (s) {
        case FileState::CollectorBehind:
        case FileState::CacheBehind:
        case FileState::MissingFromCollector:
        case FileState::MissingFromCache:
        case FileState::UnexplainedDivergence:
            return true;
        default:
            return false;
    }
}

namespace {

// Classe un fichier à partir des trois vues. `srcR`/`colR` = joignabilité de la
// source / du collecteur (le cache est toujours joignable).
FileState classify(bool srcR, bool colR, FileRow& r) {
    const bool oP = r.sourceKnown, cP = r.collectorKnown, lP = r.cacheKnown;
    const int64_t O = r.sourceSize, C = r.collectorSize, L = r.cacheSize;

    // 1. Source injoignable : impossibilité de vérifier vis-à-vis du primaire. On
    //    ne conclut jamais à « absent » ou « périmé ». On surface tout de même le
    //    signal collecteur↔cache, mais l'état reste indéterminé vs source.
    if (!srcR) {
        if (colR && cP && lP && C != L)
            r.note = "source injoignable ; collecteur et cache diffèrent (contrôle partiel)";
        else if (colR && cP && !lP)
            r.note = "source injoignable ; présent au collecteur, absent du cache (contrôle partiel)";
        else
            r.note = "source injoignable ; vérification vis-à-vis de la source impossible";
        return FileState::SourceUnreachable;
    }

    // 2. Source joignable ET le fichier y est : O fait référence.
    if (oP) {
        // Priorité aux problèmes du CACHE (ce qui est réellement analysé).
        if (!lP) return FileState::MissingFromCache;
        if (L < O) return FileState::CacheBehind;
        if (L > O) { r.note = "cache plus gros que la source (rotation/réinit ?)"; return FileState::UnexplainedDivergence; }
        // Cache == source : le cache est bon. Reste l'état du collecteur.
        if (!colR) return FileState::CollectorUnreachable;
        if (!cP)   return FileState::MissingFromCollector;
        if (C < O) return FileState::CollectorBehind;
        if (C > O) { r.note = "collecteur plus gros que la source (rotation/réinit ?)"; return FileState::UnexplainedDivergence; }
        return FileState::UpToDate;
    }

    // 3. Fichier ABSENT de la source mais présent en cache/collecteur : archive
    //    au-delà de la rétention o2switch (les vieux mois y disparaissent, on les
    //    garde). Sain : jamais « en retard vs source ». Seule action utile : si le
    //    cache ne l'a pas alors que le collecteur oui, on peut le remplir.
    r.note = "archivé (absent de la source)";
    if (!lP && colR && cP)
        return FileState::MissingFromCache;
    return FileState::UpToDate;
}

} // namespace

std::vector<FileRow> evaluate(const PathView& source,
                              const PathView& collector,
                              const PathView& cache) {
    // Union des noms vus dans les chemins JOIGNABLES (le cache l'est toujours).
    std::set<std::string> names;
    if (source.reachable)
        for (const auto& kv : source.sizes) names.insert(kv.first);
    if (collector.reachable)
        for (const auto& kv : collector.sizes) names.insert(kv.first);
    for (const auto& kv : cache.sizes) names.insert(kv.first);

    auto lookup = [](const PathView& v, bool reachable, const std::string& n,
                     int64_t& size, bool& known) {
        auto it = v.sizes.find(n);
        known = reachable && (it != v.sizes.end());
        size  = (it != v.sizes.end()) ? it->second : -1;
    };

    std::vector<FileRow> rows;
    rows.reserve(names.size());
    for (const auto& name : names) {
        FileRow r;
        r.name = name;
        lookup(source,    source.reachable,    name, r.sourceSize,    r.sourceKnown);
        lookup(collector, collector.reachable, name, r.collectorSize, r.collectorKnown);
        lookup(cache,     true,                name, r.cacheSize,     r.cacheKnown);
        r.state = classify(source.reachable, collector.reachable, r);
        rows.push_back(r);
    }
    return rows;
}

} // namespace coherence
