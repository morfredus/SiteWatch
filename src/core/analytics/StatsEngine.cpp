/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/analytics/StatsEngine.h"
#include "core/analytics/BotDetector.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace {

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Extrait le nom d'hote d'une URL de referer.
std::string hostOf(const std::string& url) {
    size_t start = url.find("://");
    if (start == std::string::npos) return "";
    start += 3;
    size_t end = url.find('/', start);
    return url.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
}

bool contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

// Une ressource technique (CSS, JS, image, police, favicon, robots, sitemap)
// n'est pas une "vraie page" : on l'exclut du Top pages.
bool isStaticAsset(const std::string& url) {
    const std::string u = toLower(url);
    static const char* exts[] = {".css", ".js", ".jpg", ".jpeg", ".png", ".gif",
                                 ".svg", ".ico", ".webp", ".woff", ".woff2",
                                 ".ttf", ".eot", ".map", ".mp4", ".webm"};
    for (const char* e : exts) {
        size_t pos = u.rfind(e);
        if (pos != std::string::npos) {
            size_t after = pos + std::strlen(e);
            if (after == u.size() || u[after] == '?') return true; // extension finale
        }
    }
    return contains(u, "/robots.txt") || contains(u, "/sitemap") ||
           contains(u, "/favicon") || contains(u, "/wp-content/uploads/");
}

} // namespace

bool StatsEngine::isAiEngine(const std::string& e) {
    return e == "Claude" || e == "OpenAI" || e == "Perplexity";
}

bool StatsEngine::isSeoEngine(const std::string& e) {
    return e == "Semrush" || e == "Ahrefs" || e == "MJ12" || e == "DotBot";
}

StatsEngine::ActivityInfo StatsEngine::classifyActivity(const LogEntry& entry) {
    const std::string u = toLower(entry.url);
    const bool errorStatus = (entry.status == 403 || entry.status == 404);

    // --- 1. Tentatives d'attaque (motifs les plus specifiques d'abord) ---
    if (contains(u, "xmlrpc.php"))                 return {ActivityType::Attack, "XML-RPC"};
    if (contains(u, "install.php"))                return {ActivityType::Attack, "install.php"};
    if (contains(u, "phpmyadmin") || contains(u, "/pma") || contains(u, "/mysql"))
                                                   return {ActivityType::Attack, "phpMyAdmin"};
    if (contains(u, "cgi-bin"))                    return {ActivityType::Attack, "CGI-BIN"};
    if (contains(u, "shell") || contains(u, "eval(") || contains(u, ".php7") ||
        contains(u, "wp-config") || contains(u, "/.env"))
                                                   return {ActivityType::Attack, "shells / eval"};
    if (contains(u, "author="))                    return {ActivityType::Attack, "énumération auteurs"};
    if (contains(u, "/backup") || contains(u, ".sql") || contains(u, ".bak"))
                                                   return {ActivityType::Attack, "backup / dump"};
    if (contains(u, "/old/"))
        return errorStatus ? ActivityInfo{ActivityType::Attack, "old (sondage)"} : ActivityInfo{};
    if (contains(u, "/wordpress/"))
        return errorStatus ? ActivityInfo{ActivityType::Attack, "wordpress (sondage)"} : ActivityInfo{};

    // Plugins et themes : un asset charge normalement (CSS/JS en 200) N'EST PAS
    // une attaque. On ne signale que les sondages ayant echoue (403/404).
    if (contains(u, "/wp-content/plugins/"))
        return errorStatus ? ActivityInfo{ActivityType::Attack, "plugins (sondage)"} : ActivityInfo{};
    if (contains(u, "/wp-content/themes/"))
        return errorStatus ? ActivityInfo{ActivityType::Attack, "themes (sondage)"} : ActivityInfo{};

    // --- 2. Activite WordPress legitime ---
    if (contains(u, "admin-ajax.php"))             return {ActivityType::Normal, "admin-ajax"};
    if (contains(u, "/wp-json"))                   return {ActivityType::Normal, "REST API"};
    if (contains(u, "wp-login"))                   return {ActivityType::Normal, "wp-login"};
    if (contains(u, "wp-admin"))                   return {ActivityType::Normal, "wp-admin"};

    return {};
}

std::string StatsEngine::classifyReferer(const std::string& referer) {
    if (referer.empty() || referer == "-") return "Direct";
    const std::string h = toLower(hostOf(referer));
    if (contains(h, "google"))     return "Google";
    if (contains(h, "bing"))       return "Bing";
    if (contains(h, "duckduckgo")) return "DuckDuckGo";
    if (contains(h, "facebook"))   return "Facebook";
    if (contains(h, "pinterest"))  return "Pinterest";
    if (contains(h, "reddit"))     return "Reddit";
    if (contains(h, "linkedin"))   return "LinkedIn";
    if (contains(h, "bsky") || contains(h, "bluesky")) return "Bluesky";
    if (contains(h, "mastodon"))   return "Mastodon";
    if (h.empty()) return "Direct";
    return "Autres";
}

void StatsEngine::ingest(const LogEntry& entry) {
    if (!entry.valid) return;

    ++stats_.totalRequests;

    // --- Periode couverte ---
    if (!entry.date.empty()) {
        if (stats_.firstDate.empty() || entry.date < stats_.firstDate) stats_.firstDate = entry.date;
        if (stats_.lastDate.empty()  || entry.date > stats_.lastDate)  stats_.lastDate  = entry.date;
    }

    // --- Codes HTTP + compteurs d'erreurs ---
    ++stats_.statusCounts[entry.status];
    if (entry.status == 404) { ++stats_.errors404; ++stats_.daily404[entry.date]; }
    else if (entry.status == 403) ++stats_.errors403;
    else if (entry.status == 500) ++stats_.errors500;

    // --- Robot ou humain ? ---
    BotResult bot = BotDetector::detect(entry.userAgent);
    if (bot.isBot) {
        ++stats_.bots;
        ++stats_.botCounts[bot.engine];
        ++stats_.dailyBots[entry.date];
        if (isAiEngine(bot.engine))  ++stats_.dailyAI[entry.date];
        if (isSeoEngine(bot.engine)) ++stats_.dailySEO[entry.date];
    } else {
        ++stats_.humans;
        ++stats_.dailyHumans[entry.date];
        // Top pages : vues humaines reussies, hors ressources techniques.
        if (entry.method == "GET" && entry.status == 200 && !isStaticAsset(entry.url))
            ++stats_.topPages[entry.url];
        ++stats_.referers[classifyReferer(entry.referer)];
    }

    // --- Activite : normale ou attaque ---
    ActivityInfo act = classifyActivity(entry);
    if (act.type == ActivityType::Normal) {
        ++stats_.normalActivity[act.label];
        ++stats_.dailyNormal[entry.date];
    } else if (act.type == ActivityType::Attack) {
        ++stats_.attackActivity[act.label];
        ++stats_.topAttacked[entry.url];   // URL attaquees = vrais scans uniquement
        ++stats_.dailyAttacks[entry.date];
    }
}
