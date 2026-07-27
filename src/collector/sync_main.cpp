/*
 * SiteWatch — outil de synchronisation avec morfCollector (headless)
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Cote FOURNISSEUR du contrat morfcollect/1. Pilote CollectorSync (le meme code
 * que la GUI) pour synchroniser la configuration, puis affiche le resultat. Sert
 * a verifier l'integration sans lancer la GUI.
 *
 * Usage :
 *   sitewatch-collector-sync <config.json> [--url http://host:port] [--fill]
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>

#include "config/Config.h"
#include "collector/CollectorSync.h"

static QTextStream& out() { static QTextStream s(stdout); return s; }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("sitewatch-collector-sync"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Synchronise la configuration SiteWatch vers morfCollector."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("config"), QStringLiteral("Fichier config.json"));
    QCommandLineOption urlOpt(QStringLiteral("url"),
        QStringLiteral("URL du collecteur (sinon decouverte morfBeacon)."), QStringLiteral("url"));
    QCommandLineOption fillOpt(QStringLiteral("fill"),
        QStringLiteral("Remplit le cache local de chaque site depuis le collecteur."));
    parser.addOption(urlOpt);
    parser.addOption(fillOpt);
    parser.process(app);

    const QStringList pos = parser.positionalArguments();
    if (pos.isEmpty()) { out() << "Usage : sitewatch-collector-sync <config.json> [--url ...]\n"; return 2; }
    const QString configPath = pos.first();

    // 1. Charger la config (attribue et persiste les UUID manquants).
    Config cfg; std::string err;
    if (!Config::load(configPath.toStdString(), cfg, err)) {
        out() << "Config illisible : " << QString::fromStdString(err) << '\n';
        return 2;
    }
    Config::save(configPath.toStdString(), cfg, err);
    out() << "Configuration : " << cfg.sites.size() << " site(s).\n";

    // 2. Localiser morfCollector.
    const QString baseUrl = collectorsync::locate(parser.value(urlOpt), 4000);
    if (baseUrl.isEmpty()) {
        out() << "Aucun morfCollector (capacite 'collection') trouve.\n"
                 "-> SiteWatch fonctionnerait en direct (aucun collecteur).\n";
        return 1;
    }
    out() << "Collecteur : " << baseUrl << '\n';

    // 3. Synchroniser (force les secrets en mode outil).
    const collectorsync::SyncResult r =
        collectorsync::synchronize(baseUrl, cfg, configPath, /*forceCredentials=*/true);
    if (!r.ok) { out() << "Echec : " << r.error << '\n'; return 1; }

    if (r.pushed) out() << "Manifeste pousse (revision " << r.revision << ", "
                        << r.sources << " source(s)).\n";
    else          out() << "Rien a faire : collecteur deja a jour (revision " << r.revision << ").\n";
    if (!r.addedLabels.isEmpty())   out() << "  Ajoutes : " << r.addedLabels.join(", ") << '\n';
    if (!r.removedLabels.isEmpty()) out() << "  Retires (fichiers CONSERVES sur le collecteur) : "
                                          << r.removedLabels.join(", ") << '\n';
    out() << "Secrets remis : " << r.credentialsPushed << ".\n";
    out() << "Metriques collecteur : "
          << QJsonDocument(r.metrics).toJson(QJsonDocument::Compact) << '\n';

    // 4. Optionnel : remplir le cache local depuis le collecteur.
    if (parser.isSet(fillOpt)) {
        for (const SiteConfig& s : cfg.sites) {
            const collectorsync::FillResult f = collectorsync::fillCache(baseUrl, s, cfg.cacheRoot);
            out() << "  cache " << QString::fromStdString(s.name) << " : "
                  << f.downloaded << " recopie(s), " << f.skipped << " deja a jour"
                  << (f.ok ? QString() : QStringLiteral(" [%1]").arg(f.error)) << '\n';
        }
    }
    return 0;
}
