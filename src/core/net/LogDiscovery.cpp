/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/net/LogDiscovery.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace logdiscovery {
namespace {

std::string toLower(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (unsigned char c : s) r += static_cast<char>(std::tolower(c));
    return r;
}

// Nom du site / préfixe privé de ses points, en minuscules (clé de comparaison
// o2switch : "morfredus.fr" et "morfredusfr" donnent la même clé).
std::string keyWithoutDots(const std::string& s) {
    std::string r;
    for (unsigned char c : s)
        if (c != '.') r += static_cast<char>(std::tolower(c));
    return r;
}

bool isDigits(const std::string& s, size_t from, size_t count) {
    if (from + count > s.size()) return false;
    for (size_t i = 0; i < count; ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[from + i]))) return false;
    return true;
}

// Nom de mois anglais abrégé (Jan..Dec), tel qu'utilisé dans les logs o2switch.
bool isMonthAbbrev(const std::string& s, size_t from) {
    static const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun",
                                    "jul", "aug", "sep", "oct", "nov", "dec"};
    if (from + 3 > s.size()) return false;
    const std::string m = toLower(s.substr(from, 3));
    for (const char* mm : months)
        if (m == mm) return true;
    return false;
}

// Position du 1er marqueur de date dans le nom (ou std::string::npos).
// Reconnaît, sur un tiret : "-Mon-YYYY", "-YYYYMMDD", puis "-YYYY".
size_t findDateMarker(const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '-') continue;
        const size_t j = i + 1;
        // -Mon-YYYY (ex. -Jul-2026)
        if (isMonthAbbrev(s, j) && j + 3 < s.size() && s[j + 3] == '-' &&
            isDigits(s, j + 4, 4))
            return i;
        // -YYYYMMDD (ex. -20260710)
        if (isDigits(s, j, 8)) return i;
        // -YYYY (ex. -2026)
        if (isDigits(s, j, 4)) return i;
    }
    return std::string::npos;
}

} // namespace

bool belongsToSite(const std::string& filename, const std::string& siteName,
                   const std::string& logMatch) {
    // Mode générique (autre hébergeur) : le nom de fichier contient le motif.
    if (!logMatch.empty())
        return toLower(filename).find(toLower(logMatch)) != std::string::npos;
    // Mode o2switch : préfixe (avant 1er point) == nom du site sans les points.
    if (siteName.empty()) return true;
    const std::string prefix = filename.substr(0, filename.find('.'));
    return keyWithoutDots(prefix) == keyWithoutDots(siteName);
}

std::string extractPrefix(const std::string& filename) {
    std::string s = filename;

    // 1) retirer l'extension .gz éventuelle.
    if (s.size() > 3 && toLower(s.substr(s.size() - 3)) == ".gz")
        s.erase(s.size() - 3);

    // 2) couper au 1er marqueur de date.
    const size_t d = findDateMarker(s);
    if (d != std::string::npos) s.erase(d);

    // 3) retirer les segments de type de log en fin de nom (.ssl.log, .log…).
    static const char* logWords[] = {"log", "ssl", "access", "error",
                                     "nginx", "apache"};
    bool stripped = true;
    while (stripped) {
        stripped = false;
        const size_t dot = s.rfind('.');
        if (dot == std::string::npos) break;
        const std::string tail = toLower(s.substr(dot + 1));
        for (const char* w : logWords) {
            if (tail == w) {
                s.erase(dot);
                stripped = true;
                break;
            }
        }
    }
    return s;
}

Report analyze(const std::vector<std::string>& files, const SiteConfig& site) {
    Report report;
    report.totalGz = static_cast<int>(files.size());

    for (const std::string& f : files)
        if (belongsToSite(f, site.name, site.logMatch))
            report.matched.push_back(f);

    if (!report.matched.empty()) {
        report.outcome = Outcome::Matched;
        return report;
    }
    if (files.empty()) {
        report.outcome = Outcome::NoFilesInDir;
        return report;
    }

    // Des fichiers existent mais aucun ne correspond : tenter une proposition.
    report.outcome = Outcome::NoneMatch;

    // Regrouper par préfixe déduit, en conservant un libellé d'origine.
    std::map<std::string, int>         counts;   // clé minuscule -> nombre
    std::map<std::string, std::string> labels;   // clé minuscule -> libellé affiché
    for (const std::string& f : files) {
        const std::string prefix = extractPrefix(f);
        if (prefix.empty()) continue;
        const std::string key = toLower(prefix);
        counts[key]++;
        labels.emplace(key, prefix);
    }
    if (counts.empty()) return report;

    // Préfixe dominant.
    auto best = std::max_element(counts.begin(), counts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    const std::string bestKey   = best->first;
    const std::string bestLabel = labels[bestKey];
    const int         bestCount = best->second;

    // Fiabilité : majorité nette (>= 60 %), préfixe spécifique, différent du
    // filtre courant, et qui retiendrait effectivement des fichiers.
    const bool hasLetter =
        std::any_of(bestLabel.begin(), bestLabel.end(),
                    [](unsigned char c) { return std::isalpha(c) != 0; });
    const bool majority = bestCount * 10 >= report.totalGz * 6;
    const bool specific = bestLabel.size() >= 3 && hasLetter;
    const bool different = toLower(site.logMatch) != bestKey;

    if (majority && specific && different) {
        int wouldMatch = 0;
        for (const std::string& f : files)
            if (belongsToSite(f, site.name, bestLabel)) ++wouldMatch;
        if (wouldMatch > 0) {
            report.suggestion.valid = true;
            report.suggestion.filter = bestLabel;
            report.suggestion.wouldMatch = wouldMatch;
        }
    }
    return report;
}

} // namespace logdiscovery
