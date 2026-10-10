/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "publish/SiteAnalysis.h"

#include "core/analytics/StatsEngine.h"
#include "core/cache/CacheManager.h"
#include "core/io/GzReader.h"
#include "core/net/LogDiscovery.h"
#include "core/parser/ApacheLogParser.h"

#include <QDate>
#include <QJsonArray>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace siteanalysis {
namespace {

// Les logs en cache qui appartiennent au site (un dossier peut en melanger plusieurs).
std::vector<std::string> siteLogs(const std::string& cacheRoot, const SiteConfig& site) {
    std::vector<std::string> logs = CacheManager(cacheRoot).localLogs(site.name);
    if (!site.name.empty()) {
        logs.erase(std::remove_if(logs.begin(), logs.end(), [&](const std::string& path) {
            return !logdiscovery::belongsToSite(fs::path(path).filename().string(),
                                                site.name, site.logMatch);
        }), logs.end());
    }
    return logs;
}

long sumAttacks(const Stats& s) {
    long total = 0;
    for (const auto& [label, count] : s.attackActivity) total += count;
    return total;
}

// Le rapport est une synthese Web, pas une copie des journaux : un gros site depasserait
// la taille maximale d'une requete HTTP et ne serait plus publie du tout.
QJsonObject rankedMapJson(const std::map<std::string, long>& values, size_t maxItems = 20) {
    std::vector<std::pair<std::string, long>> ranked(values.begin(), values.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    QJsonObject out;
    const size_t kMaximumItems = maxItems;
    constexpr int kMaximumLabelLength = 180;
    for (size_t i = 0; i < ranked.size() && i < kMaximumItems; ++i) {
        QString label = QString::fromStdString(ranked[i].first);
        if (label.size() > kMaximumLabelLength)
            label = label.left(kMaximumLabelLength - 1) + QChar(0x2026);
        out[label] = static_cast<double>(ranked[i].second);
    }
    return out;
}

// Cumule les pages (ou les robots) vues sur les jours [from, to] (chaines "AAAA-MM-JJ").
std::map<std::string, long> sumDays(const std::map<std::string, std::map<std::string, long>>& perDay,
                                    const QDate& from, const QDate& to) {
    std::map<std::string, long> out;
    for (const auto& [day, items] : perDay) {
        const QDate d = QDate::fromString(QString::fromStdString(day), Qt::ISODate);
        if (!d.isValid() || d < from || d > to) continue;
        for (const auto& [name, n] : items) out[name] += n;
    }
    return out;
}

QJsonArray arrayOf(const std::array<long, 24>& a) {
    QJsonArray out;
    for (long v : a) out.append(static_cast<double>(v));
    return out;
}

// Familles de codes HTTP (2xx/3xx/4xx/5xx) : lecture rapide de la sante du site.
QJsonObject statusClassesJson(const Stats& s) {
    long c[6] = {0, 0, 0, 0, 0, 0};
    for (const auto& [code, n] : s.statusCounts)
        if (code >= 100 && code < 600) c[code / 100] += n;
    return QJsonObject{{"2xx", static_cast<double>(c[2])}, {"3xx", static_cast<double>(c[3])},
                       {"4xx", static_cast<double>(c[4])}, {"5xx", static_cast<double>(c[5])}};
}

// Repli sans magasin : 90 derniers jours seulement.
QJsonObject dailyMapJson(const std::map<std::string, long>& values) {
    QJsonObject out;
    constexpr size_t kMaximumDays = 90;
    const size_t skip = values.size() > kMaximumDays ? values.size() - kMaximumDays : 0;
    size_t index = 0;
    for (const auto& [day, count] : values) {
        if (index++ >= skip) out[QString::fromStdString(day)] = static_cast<double>(count);
    }
    return out;
}

} // namespace

QMap<QString, qint64> siteLogSizes(const std::string& cacheRoot, const SiteConfig& site) {
    QMap<QString, qint64> sizes;
    for (const std::string& path : siteLogs(cacheRoot, site)) {
        std::error_code ec;
        sizes.insert(QString::fromStdString(fs::path(path).filename().string()),
                     static_cast<qint64>(fs::file_size(path, ec)));
    }
    return sizes;
}

Result computeSite(const std::string& cacheRoot, const SiteConfig& site,
                   const std::string& fromStr, const std::string& toStr,
                   bool wantFull, bool keepEntries) {
    Result r;
    const std::vector<std::string> logs = siteLogs(cacheRoot, site);
    r.sizes = siteLogSizes(cacheRoot, site);
    for (auto it = r.sizes.begin(); it != r.sizes.end(); ++it)
        r.totalBytes += static_cast<unsigned long long>(it.value());

    // Verite web : un moteur sans filtre de date, nourri dans la MEME lecture.
    StatsEngine engine, fullEngine;
    for (const std::string& file : logs) {
        GzReader::readLines(file, [&](const std::string& line) {
            LogEntry e = ApacheLogParser::parseLine(line);
            if (!e.valid) return;
            if (wantFull) fullEngine.ingest(e);
            if (e.date >= fromStr && e.date <= toStr) {
                engine.ingest(e);
                if (keepEntries) r.entries.push_back(e);   // detail au double-clic
            }
        });
    }
    r.stats = engine.result();
    if (wantFull) r.full = fullEngine.result();
    return r;
}

Result analyzeSite(const std::string& cacheRoot, const SiteConfig& site,
                   const std::string& fromStr, const std::string& toStr,
                   WebAuthorityStore* web, bool keepEntries) {
    const QString siteId = QString::fromStdString(site.id);
    const QMap<QString, qint64> sizes = siteLogSizes(cacheRoot, site);
    const bool consolidate = web && !siteId.isEmpty() && !sizes.isEmpty()
                             && web->needsConsolidation(siteId, sizes);
    Result r = computeSite(cacheRoot, site, fromStr, toStr, consolidate, keepEntries);
    if (consolidate) {
        web->consolidate(siteId, r.full, r.sizes);
        r.consolidated = true;
    }
    return r;
}

QJsonObject buildReport(const SiteConfig& site, const Stats& s, WebAuthorityStore* web) {
    // Derniere date vue dans la periode : les comparaisons « 7 derniers jours / 7 precedents »
    // s'ancrent dessus, pas sur la date du jour (un site peut avoir des logs en retard).
    const QDate lastD = QDate::fromString(QString::fromStdString(s.lastDate), Qt::ISODate);
    QJsonObject stats{{"requests", static_cast<double>(s.totalRequests)},
        {"bots", static_cast<double>(s.bots)}, {"attacks", static_cast<double>(sumAttacks(s))},
        {"errors_404", static_cast<double>(s.errors404)}, {"errors_403", static_cast<double>(s.errors403)},
        {"errors_500", static_cast<double>(s.errors500)},
        {"top_pages", rankedMapJson(s.topPages)}, {"top_attacked", rankedMapJson(s.topAttacked)},
        {"bot_counts", rankedMapJson(s.botCounts)}, {"daily_humans", dailyMapJson(s.dailyHumans)},
        {"daily_bots", dailyMapJson(s.dailyBots)}, {"daily_ai", dailyMapJson(s.dailyAI)},
        {"daily_seo", dailyMapJson(s.dailySEO)}, {"daily_404", dailyMapJson(s.daily404)},
        {"daily_403", dailyMapJson(s.daily403)}, {"daily_500", dailyMapJson(s.daily500)},
        {"referers", rankedMapJson(s.referers)},
        {"attack_activity", rankedMapJson(s.attackActivity)},
        {"normal_activity", rankedMapJson(s.normalActivity)},
        {"status_classes", statusClassesJson(s)},
        {"top_404", rankedMapJson(s.topBroken)},
        {"hourly", QJsonObject{{"humans", arrayOf(s.hourHumans)}, {"bots", arrayOf(s.hourBots)},
                                {"attacks", arrayOf(s.hourAttacks)}}},
        {"pages_recent", rankedMapJson(sumDays(s.dailyPages, lastD.addDays(-6), lastD), 50)},
        {"pages_prior", rankedMapJson(sumDays(s.dailyPages, lastD.addDays(-13), lastD.addDays(-7)), 50)},
        {"bots_recent", rankedMapJson(sumDays(s.dailyBotEngines, lastD.addDays(-6), lastD), 50)},
        {"bots_prior", rankedMapJson(sumDays(s.dailyBotEngines, QDate(2000, 1, 1), lastD.addDays(-7)), 50)},
        {"daily_attacks", dailyMapJson(s.dailyAttacks)}, {"daily_normal", dailyMapJson(s.dailyNormal)}};

    // Les series de la verite consolidee (tout l'historique) remplacent celles de la
    // periode, plafonnees a 90 jours. Les champs de fraicheur disent l'age des donnees.
    qint64 consolidatedAt = 0;
    QString upToDay = QString::fromStdString(s.lastDate);
    if (web && web->isOpen()) {
        const QString sid = QString::fromStdString(site.id);
        const QJsonObject daily = web->daily(sid);
        for (auto it = daily.begin(); it != daily.end(); ++it)
            stats[it.key()] = it.value();
        consolidatedAt = web->consolidatedAt(sid);
        if (!web->lastDay(sid).isEmpty())
            upToDay = web->lastDay(sid);
    }
    return QJsonObject{{"site_id", QString::fromStdString(site.id)},
        {"site_label", QString::fromStdString(site.name)},
        {"from", QString::fromStdString(s.firstDate)}, {"to", QString::fromStdString(s.lastDate)},
        {"source_up_to", upToDay}, {"consolidated_at", static_cast<double>(consolidatedAt)},
        {"stats", stats}};
}

bool postReport(const QString& analyticsBase, const QJsonObject& report, QString& error) {
    QNetworkAccessManager nam;
    QNetworkRequest request(QUrl(analyticsBase + QStringLiteral("/sitewatch/ingest")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = nam.post(request, QJsonDocument(report).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(20000);
    loop.exec();
    if (reply->isRunning())
        reply->abort();
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = reply->error() == QNetworkReply::NoError && http == 200;
    if (!ok)
        error = http ? QStringLiteral("HTTP %1").arg(http) : reply->errorString();
    reply->deleteLater();
    return ok;
}

} // namespace siteanalysis
