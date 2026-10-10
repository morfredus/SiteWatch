/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

#include <string>
#include <vector>

#include "config/Config.h"
#include "core/model/LogEntry.h"
#include "core/model/Stats.h"
#include "webstore/WebAuthorityStore.h"

// Analyse d'un site et publication de son rapport, SANS interface : partage entre la
// GUI et l'outil headless (sitewatch-publish) pour qu'il n'y ait qu'une seule source.
namespace siteanalysis {

struct Result {
    Stats stats;                       // periode demandee
    QMap<QString, qint64> sizes;       // logs lus : nom -> taille
    unsigned long long totalBytes = 0;
    bool consolidated = false;         // le magasin web a ete mis a jour
    std::vector<LogEntry> entries;     // seulement si keepEntries
    Stats full;                        // tout l'historique (seulement si wantFull)
};

// Calcul PUR : lit les logs en cache et rend les statistiques de la periode (et, si `wantFull`,
// de tout l'historique), sans toucher a aucune base. Utilisable dans un fil de travail ; la
// consolidation dans le magasin (connexion SQLite liee au fil de l'interface) se fait ensuite
// avec `WebAuthorityStore::consolidate(site, r.full, r.sizes)`.
Result computeSite(const std::string& cacheRoot, const SiteConfig& site,
                   const std::string& fromStr, const std::string& toStr,
                   bool wantFull, bool keepEntries = false);

// Lit les logs en cache du site UNE fois : alimente la periode [from, to] et, si un
// fichier est nouveau ou a change de taille, le magasin web sur tout l'historique.
Result analyzeSite(const std::string& cacheRoot, const SiteConfig& site,
                   const std::string& fromStr, const std::string& toStr,
                   WebAuthorityStore* web, bool keepEntries = false);

// Tailles des logs en cache d'un site (meme filtre que l'analyse).
QMap<QString, qint64> siteLogSizes(const std::string& cacheRoot, const SiteConfig& site);

// Corps de POST /sitewatch/ingest : synthese de la periode, series quotidiennes
// consolidees (tout l'historique) et champs de fraicheur.
QJsonObject buildReport(const SiteConfig& site, const Stats& stats, WebAuthorityStore* web);

// POST synchrone (outil headless). `analyticsBase` sans slash final.
bool postReport(const QString& analyticsBase, const QJsonObject& report, QString& error);

} // namespace siteanalysis
