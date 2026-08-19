/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Contrôle de cohérence en ligne de commande (chantier B1) : pour chaque site,
 * confronte o2switch, le collecteur sélectionné et le cache local, en métadonnées
 * seulement (aucun .gz téléchargé). Sert de vérification end-to-end réelle avant
 * tout câblage d'interface.
 *
 *   sitewatch-coherence <config.json> [--url http://pi4dev:8792] [--site nom]
 *
 * Sans --url, découvre le collecteur via morfBeacon. Code retour : 0 si aucune
 * divergence exploitable, 1 si au moins une ligne est « actionnable ».
 */

#include "coherence/CoherenceCheck.h"
#include "collector/CollectorClient.h"
#include "config/Config.h"

#include <QCoreApplication>
#include <QString>

#include <iostream>
#include <string>

using namespace coherence;

namespace {

std::string sz(int64_t v, bool known) {
    return known ? std::to_string(v) : std::string("  —  ");
}

void printReport(const SiteReport& rep) {
    std::cout << "\n== " << rep.site << " ==\n";
    std::cout << "  source o2switch : "
              << (rep.sourceReachable ? "joignable"
                                      : "INJOIGNABLE (" + rep.sourceError + ")") << "\n";
    std::cout << "  collecteur      : "
              << (rep.collectorReachable ? "joignable"
                                         : "INJOIGNABLE (" + rep.collectorError + ")") << "\n";
    if (rep.rows.empty()) {
        std::cout << "  (aucun fichier attendu)\n";
        return;
    }
    std::cout << "  fichier | source | collec. | cache | état\n";
    for (const auto& r : rep.rows) {
        std::cout << "  " << (isActionable(r.state) ? "! " : "  ")
                  << sz(r.sourceSize, r.sourceKnown) << " | "
                  << sz(r.collectorSize, r.collectorKnown) << " | "
                  << sz(r.cacheSize, r.cacheKnown) << " | "
                  << toString(r.state) << "  " << r.name;
        if (!r.note.empty()) std::cout << "  (" << r.note << ")";
        std::cout << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);   // requis : CollectorClient utilise QEventLoop

    std::string configPath, onlySite;
    QString url;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--url" && i + 1 < argc)        url = QString::fromLocal8Bit(argv[++i]);
        else if (a == "--site" && i + 1 < argc)  onlySite = argv[++i];
        else if (!a.empty() && a[0] != '-')      configPath = a;
    }
    if (configPath.empty()) {
        std::cerr << "usage: sitewatch-coherence <config.json> [--url URL] [--site nom]\n";
        return 2;
    }

    Config cfg;
    std::string err;
    if (!Config::load(configPath, cfg, err)) {
        std::cerr << "config illisible : " << err << "\n";
        return 2;
    }

    // Résolution du collecteur de LECTURE : --url, sinon config, sinon découverte.
    if (url.isEmpty() && !cfg.collectorUrl.empty())
        url = QString::fromStdString(cfg.collectorUrl);
    if (url.isEmpty()) {
        CollectorClient::Discovered d;
        QString derr;
        if (CollectorClient::discover(3000, d, derr))
            url = d.baseUrl();
    }
    std::cout << "Collecteur de lecture : "
              << (url.isEmpty() ? std::string("(aucun)") : url.toStdString()) << "\n";

    int actionable = 0;
    for (const SiteConfig& s : cfg.sites) {
        if (!onlySite.empty() && s.name != onlySite)
            continue;
        const SiteReport rep = checkSite(s, url, cfg.cacheRoot);
        printReport(rep);
        for (const auto& r : rep.rows)
            if (isActionable(r.state)) ++actionable;
    }

    std::cout << "\n" << (actionable == 0
        ? "Aucune divergence exploitable."
        : std::to_string(actionable) + " ligne(s) appellent une action.") << "\n";
    return actionable == 0 ? 0 : 1;
}
