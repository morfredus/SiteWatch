/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/parser/ApacheLogParser.h"
#include <array>
#include <cstdlib>

namespace {

// Lit un champ delimite par des guillemets a partir de la position pos.
// Renvoie le contenu (sans les guillemets) et avance pos apres le guillemet
// fermant. Renvoie false si aucun champ entre guillemets n'est trouve.
bool readQuoted(const std::string& s, size_t& pos, std::string& out) {
    // On se place sur le prochain guillemet ouvrant.
    while (pos < s.size() && s[pos] != '"') ++pos;
    if (pos >= s.size()) return false;
    ++pos; // saute le guillemet ouvrant
    size_t start = pos;
    while (pos < s.size() && s[pos] != '"') ++pos;
    if (pos >= s.size()) return false;
    out = s.substr(start, pos - start);
    ++pos; // saute le guillemet fermant
    return true;
}

// Lit un champ delimite par des espaces a partir de la position pos.
std::string readToken(const std::string& s, size_t& pos) {
    while (pos < s.size() && s[pos] == ' ') ++pos;
    size_t start = pos;
    while (pos < s.size() && s[pos] != ' ') ++pos;
    return s.substr(start, pos - start);
}

// Lit un champ entre crochets [ ... ].
bool readBracket(const std::string& s, size_t& pos, std::string& out) {
    while (pos < s.size() && s[pos] != '[') ++pos;
    if (pos >= s.size()) return false;
    ++pos;
    size_t start = pos;
    while (pos < s.size() && s[pos] != ']') ++pos;
    if (pos >= s.size()) return false;
    out = s.substr(start, pos - start);
    ++pos;
    return true;
}

int monthToNumber(const std::string& mon) {
    static const std::array<const char*, 12> names = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; ++i)
        if (mon == names[i]) return i + 1;
    return 0;
}

} // namespace

std::string ApacheLogParser::normalizeDate(const std::string& apacheDate) {
    // Format attendu : "06/Jul/2026:10:15:32 +0200"
    if (apacheDate.size() < 11) return "";
    std::string day   = apacheDate.substr(0, 2);
    std::string mon   = apacheDate.substr(3, 3);
    std::string year  = apacheDate.substr(7, 4);
    int m = monthToNumber(mon);
    if (m == 0) return "";
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%s-%02d-%s", year.c_str(), m, day.c_str());
    return std::string(buf);
}

LogEntry ApacheLogParser::parseLine(const std::string& line) {
    LogEntry e;
    if (line.empty()) return e;

    size_t pos = 0;

    // 1. IP client
    e.ip = readToken(line, pos);
    // 2. identd (ignore) et 3. userid (ignore)
    readToken(line, pos);
    readToken(line, pos);

    // 4. Date entre crochets
    std::string rawDate;
    if (!readBracket(line, pos, rawDate)) return e;
    e.date = normalizeDate(rawDate);
    // Heure : format "JJ/Mon/AAAA:HH:MM:SS ..." -> HH à l'index 12.
    if (rawDate.size() >= 14) e.hour = std::atoi(rawDate.substr(12, 2).c_str());

    // 5. Requete "METHODE URL PROTOCOLE" entre guillemets
    std::string request;
    if (!readQuoted(line, pos, request)) return e;
    {
        size_t p = 0;
        e.method = readToken(request, p);
        e.url    = readToken(request, p);
        // le protocole (HTTP/1.1) n'est pas conserve
    }

    // 6. Code de statut
    std::string statusTok = readToken(line, pos);
    e.status = std::atoi(statusTok.c_str());

    // 7. Taille de la reponse ("-" possible)
    std::string sizeTok = readToken(line, pos);
    e.size = (sizeTok == "-") ? 0 : std::atoll(sizeTok.c_str());

    // 8. Referer entre guillemets
    readQuoted(line, pos, e.referer);
    // 9. User-Agent entre guillemets
    readQuoted(line, pos, e.userAgent);

    // Une ligne est valide si l'essentiel est present.
    e.valid = !e.ip.empty() && !e.date.empty() && e.status != 0;
    return e;
}
