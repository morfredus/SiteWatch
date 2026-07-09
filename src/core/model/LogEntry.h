/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>

// -----------------------------------------------------------------------------
// LogEntry : represente UNE ligne de log Apache/LiteSpeed au format "combined".
//
// Exemple de ligne brute :
// 66.249.66.1 - - [06/Jul/2026:10:15:32 +0200] "GET /page HTTP/1.1" 200 1234
//   "https://google.com" "Mozilla/5.0 (compatible; Googlebot/2.1; ...)"
//
// Cette structure appartient au coeur : elle ne connait ni Qt ni le reseau.
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string ip;          // adresse IP du client
    std::string date;        // date normalisee "AAAA-MM-JJ" (pour les regroupements)
    int         hour = -1;   // heure (0-23) de la requete
    std::string method;      // GET, POST, HEAD...
    std::string url;         // chemin demande
    int         status = 0;  // code HTTP (200, 404, 403, 500...)
    long long   size   = 0;  // taille de la reponse en octets
    std::string referer;     // page d'origine ("-" si absente)
    std::string userAgent;   // signature du navigateur ou du robot

    bool valid = false;      // false si la ligne n'a pas pu etre analysee
};
