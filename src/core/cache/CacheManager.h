/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// CacheManager : gere le dossier de cache local des logs.
//
// Arborescence attendue :
//     D:\SiteWatch\<site>\Jun-2026.gz
//                        \Jul-2026.gz
//
// Role actuel (Phase 1) : lister les fichiers .gz deja presents pour un site.
// Role futur (Phase 2)  : comparer avec la liste distante et savoir quels
//                         fichiers manquent (pour ne telecharger que le nouveau).
// -----------------------------------------------------------------------------
class CacheManager {
public:
    explicit CacheManager(std::string cacheRoot);

    // Renvoie le chemin du dossier local d'un site (le cree si absent).
    std::string siteDir(const std::string& siteName) const;

    // Liste les fichiers .gz presents localement pour un site (chemins complets).
    std::vector<std::string> localLogs(const std::string& siteName) const;

private:
    std::string cacheRoot_;
};
