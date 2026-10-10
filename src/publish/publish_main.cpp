/*
 * SiteWatch - outil de publication headless
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Fait SANS interface ce que SiteWatch fait a l'ouverture, pour que morfAnalytics reste
 * a jour meme quand la GUI n'est pas lancee pendant des jours :
 *
 *   1. si un morfCollector est joignable : synchroniser la configuration et recopier
 *      dans le cache les logs qu'il a recoltes (seulement ce qui manque ou a grossi) ;
 *   2. consolider la verite web de chaque site (magasin SQLite, tout l'historique) ;
 *   3. publier chaque site vers morfAnalytics, puis le rattrapage GitHub.
 *
 * Il utilise le meme code que la GUI et ecrit dans les MEMES magasins : lancer la GUI
 * ensuite n'a rien a refaire. Sans collecteur ni morfAnalytics il se contente de
 * consolider le cache local (SiteWatch reste autonome).
 *
 * Usage :
 *   sitewatch-publish [config.json] [--collector URL] [--analytics URL]
 *                     [--days N] [--no-collector]
 * Code retour : 0 tout va bien, 1 au moins une etape a echoue, 2 config illisible.
 */

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDate>
#include <QStandardPaths>
#include <QTextStream>

#include "collector/CollectorSync.h"
#include "config/Config.h"
#include "github/GitHubAuthorityStore.h"
#include "github/GitHubSync.h"
#include "publish/SiteAnalysis.h"
#include "webstore/WebAuthorityStore.h"

static QTextStream& out() { static QTextStream s(stdout); return s; }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // Meme nom que la GUI, sans organisation : meme dossier de configuration par defaut.
    QCoreApplication::setApplicationName(QStringLiteral("SiteWatch"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Consolide les logs de SiteWatch et publie vers morfAnalytics, sans interface."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("config"),
        QStringLiteral("Fichier config.json (defaut : celui de la GUI)."), QStringLiteral("[config]"));
    QCommandLineOption collectorOpt(QStringLiteral("collector"),
        QStringLiteral("URL de morfCollector (sinon config, sinon decouverte morfBeacon)."),
        QStringLiteral("url"));
    QCommandLineOption analyticsOpt(QStringLiteral("analytics"),
        QStringLiteral("URL de morfAnalytics (sinon config)."), QStringLiteral("url"));
    QCommandLineOption daysOpt(QStringLiteral("days"),
        QStringLiteral("Periode de la synthese publiee, en jours (defaut 60)."),
        QStringLiteral("n"), QStringLiteral("60"));
    QCommandLineOption noCollectorOpt(QStringLiteral("no-collector"),
        QStringLiteral("Ne pas contacter de collecteur."));
    parser.addOption(collectorOpt);
    parser.addOption(analyticsOpt);
    parser.addOption(daysOpt);
    parser.addOption(noCollectorOpt);
    parser.process(app);

    QString configPath = parser.positionalArguments().value(0);
    if (configPath.isEmpty())
        configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                     + QStringLiteral("/config.json");

    Config cfg; std::string err;
    if (!Config::load(configPath.toStdString(), cfg, err)) {
        out() << "Config illisible (" << configPath << ") : " << QString::fromStdString(err) << '\n';
        return 2;
    }
    out() << "Configuration : " << configPath << " - " << cfg.sites.size() << " site(s).\n";
    int failures = 0;

    // 1. Collecteur : alignement de la config puis recopie de ce qui manque au cache.
    QString collector;
    if (!parser.isSet(noCollectorOpt)) {
        collector = parser.value(collectorOpt);
        if (collector.isEmpty())
            collector = QString::fromStdString(cfg.collectorUrl);
        if (collector.isEmpty())
            collector = collectorsync::locate(QString(), 4000);
    }
    if (collector.isEmpty()) {
        out() << "Pas de collecteur : consolidation du cache local seulement.\n";
    } else {
        const collectorsync::SyncResult sr =
            collectorsync::synchronize(collector, cfg, configPath, /*forceCredentials=*/false);
        if (!sr.ok) {
            out() << "Collecteur " << collector << " : synchronisation impossible (" << sr.error << ").\n";
            collector.clear();   // pas de lecture depuis un collecteur qui ne repond pas
            ++failures;
        } else {
            out() << "Collecteur " << collector << " : synchronise (revision " << sr.revision << ").\n";
            for (const SiteConfig& s : cfg.sites) {
                const collectorsync::FillResult f = collectorsync::fillCache(collector, s, cfg.cacheRoot);
                out() << "  " << QString::fromStdString(s.name) << " : " << f.downloaded
                      << " recopie(s), " << f.skipped << " deja a jour"
                      << (f.ok ? QString() : QStringLiteral(" [%1]").arg(f.error)) << '\n';
                if (!f.ok) ++failures;
            }
        }
    }

    // 2. Verite web + publication vers morfAnalytics.
    QString analytics = parser.value(analyticsOpt);
    if (analytics.isEmpty())
        analytics = QString::fromStdString(cfg.analyticsUrl);
    while (analytics.endsWith(QLatin1Char('/')))
        analytics.chop(1);
    if (analytics.isEmpty())
        out() << "Pas d'adresse morfAnalytics (--analytics ou analyticsUrl) : rien ne sera publie.\n";

    WebAuthorityStore web(QString::fromStdString(cfg.cacheRoot) + QStringLiteral("/web/web.sqlite"));
    const bool webOk = web.open();
    if (!webOk) {
        out() << "Magasin web indisponible : " << web.lastError() << '\n';
        ++failures;
    }
    const int days = std::max(1, parser.value(daysOpt).toInt());
    const std::string toStr = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")).toStdString();
    const std::string fromStr = QDate::currentDate().addDays(-days).toString(QStringLiteral("yyyy-MM-dd")).toStdString();

    for (const SiteConfig& s : cfg.sites) {
        const siteanalysis::Result r = siteanalysis::analyzeSite(
            cfg.cacheRoot, s, fromStr, toStr, webOk ? &web : nullptr);
        if (r.sizes.isEmpty()) {
            out() << "  " << QString::fromStdString(s.name) << " : aucun log en cache.\n";
            continue;
        }
        out() << "  " << QString::fromStdString(s.name) << " : " << r.sizes.size() << " fichier(s)"
              << (r.consolidated ? ", historique consolide" : ", historique deja a jour");
        if (!analytics.isEmpty()) {
            QString perr;
            const bool ok = siteanalysis::postReport(analytics,
                siteanalysis::buildReport(s, r.stats, webOk ? &web : nullptr), perr);
            out() << (ok ? ", publie" : QStringLiteral(", PUBLICATION ECHOUEE (%1)").arg(perr));
            if (!ok) ++failures;
        }
        out() << '\n';
    }

    // 3. GitHub : rattrapage depuis le collecteur, puis publication de la verite.
    if (cfg.github.enabled && !cfg.github.owner.empty()) {
        GitHubAuthorityStore gh(QString::fromStdString(cfg.cacheRoot) + QStringLiteral("/github/github.sqlite"));
        if (!gh.open()) {
            out() << "Magasin GitHub indisponible : " << gh.lastError() << '\n';
            ++failures;
        } else {
            const int newDays = collector.isEmpty() ? 0 : githubsync::catchUp(cfg, collector, gh);
            out() << "GitHub : " << newDays << " jour(s) nouveau(x) depuis le collecteur";
            if (!analytics.isEmpty() && gh.hasConsolidatedData()) {
                bool ok = false;
                const QString msg = githubsync::publish(analytics, gh, ok);
                out() << ". " << msg;
                if (!ok) ++failures;
            }
            out() << '\n';
        }
    }

    out() << (failures ? "Termine avec des erreurs.\n" : "Termine.\n");
    return failures ? 1 : 0;
}
