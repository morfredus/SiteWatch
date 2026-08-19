/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <map>
#include <string>
#include <vector>

#include "coherence/CoherenceEngine.h"
#include "config/Config.h"

// -----------------------------------------------------------------------------
// CoherenceCheck : l'ADAPTATEUR d'E/S du contrôle de cohérence (chantier B1).
//
// Construit les trois vues RÉELLES d'un site puis les confie au moteur pur
// (CoherenceEngine). Réutilise l'existant, sans rien télécharger :
//   - source o2switch : SftpClient::listLogs (listage SFTP, tailles seules) ;
//   - collecteur      : CollectorClient::getObjects (métadonnées d'index) ;
//   - cache local     : scan de cacheRoot/<site> (tailles du système de fichiers).
//
// Le filtrage de site réutilise logdiscovery::belongsToSite (même règle que la
// collecte). Une source injoignable est signalée comme telle (reachable=false),
// jamais confondue avec une absence de fichiers.
// -----------------------------------------------------------------------------
namespace coherence {

struct SiteReport {
    std::string          site;
    bool                 sourceReachable    = false;
    bool                 collectorReachable = false;
    std::string          sourceError;      // vide si joignable
    std::string          collectorError;   // vide si joignable
    std::vector<FileRow> rows;             // un par fichier attendu (cf. moteur)
    // nom d'origine -> object_id du collecteur : pour proposer un remplissage
    // ciblé du cache depuis une copie conservée (action « depuis collecteur »).
    std::map<std::string, std::string> collectorObjectIds;
};

// Évalue la cohérence d'UN site contre le collecteur `collectorBaseUrl` (celui
// choisi pour la lecture) et le cache local `cacheRoot`. Métadonnées seulement.
SiteReport checkSite(const SiteConfig& site, const QString& collectorBaseUrl,
                     const std::string& cacheRoot);

} // namespace coherence
