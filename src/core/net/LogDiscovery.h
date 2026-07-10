/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <vector>
#include "config/Config.h"

// -----------------------------------------------------------------------------
// LogDiscovery : logique PURE de sélection et de détection des fichiers de logs.
//
// Ce module ne connaît NI Qt NI le réseau : il ne manipule que des noms de
// fichiers (chaînes) et la configuration d'un site. Il reste donc portable
// (Windows/Linux) et facilement testable sans interface ni serveur SFTP.
//
// Il répond à deux besoins :
//   1. décider si un fichier appartient à un site (filtre avancé ou détection
//      automatique o2switch) — logique partagée entre le téléchargement et
//      l'analyse ;
//   2. analyser la liste des fichiers présents sur le serveur pour distinguer
//      « aucun fichier » / « fichiers présents mais aucun ne correspond » et,
//      dans ce dernier cas, proposer automatiquement le meilleur filtre.
// -----------------------------------------------------------------------------
namespace logdiscovery {

// Un fichier de log appartient-il au site ?
//   - logMatch non vide : mode générique (autre hébergeur), le nom contient le
//     motif (comparaison insensible à la casse) ;
//   - logMatch vide : mode o2switch, le préfixe (avant le 1er point) doit être
//     égal au nom du site privé de ses points (exclut les sous-domaines).
bool belongsToSite(const std::string& filename, const std::string& siteName,
                   const std::string& logMatch);

// Extrait le préfixe « significatif » d'un nom de log, utilisé pour deviner un
// filtre. On retire .gz, on coupe au 1er marqueur de date (-YYYYMMDD, -Mon-YYYY
// ou -YYYY), puis on retire un suffixe .ssl.log / .log / .access / .error.
//   "tabacclaouey.fr.ssl.log-20260710.gz" -> "tabacclaouey.fr"
//   "monsitefr.ssl.log-20260710.gz"       -> "monsitefr"
std::string extractPrefix(const std::string& filename);

// Situation constatée sur le serveur vis-à-vis du site.
enum class Outcome {
    Matched,       // au moins un fichier correspond au filtre/domaine actuel
    NoneMatch,     // des .gz existent mais aucun ne correspond au filtre actuel
    NoFilesInDir,  // le dossier distant ne contient aucun .gz
};

// Proposition automatique de filtre (uniquement si jugée fiable).
struct Suggestion {
    bool        valid = false;   // true si une proposition fiable a été trouvée
    std::string filter;          // filtre proposé (ex. "tabacclaouey.fr")
    int         wouldMatch = 0;  // nombre de fichiers que ce filtre retiendrait
};

// Résultat complet de l'analyse de la liste distante.
struct Report {
    Outcome                  outcome = Outcome::NoFilesInDir;
    std::vector<std::string> matched;      // fichiers retenus pour ce site
    int                      totalGz = 0;  // total de .gz dans le dossier
    Suggestion               suggestion;   // proposition auto (si outcome == NoneMatch)
};

// Analyse la liste des noms de .gz présents dans le dossier distant.
// 'files' = noms de fichiers (sans chemin). Renvoie le diagnostic et, en cas de
// NoneMatch, une éventuelle proposition de filtre.
Report analyze(const std::vector<std::string>& files, const SiteConfig& site);

} // namespace logdiscovery
