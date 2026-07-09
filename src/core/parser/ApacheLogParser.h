/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include "core/model/LogEntry.h"

// -----------------------------------------------------------------------------
// ApacheLogParser : transforme une ligne de texte brute en LogEntry.
//
// Format supporte : "combined" (Apache / LiteSpeed), le format standard o2switch.
// L'analyse est faite a la main (pas de regex) pour rester tres rapide,
// meme sur des fichiers de plusieurs centaines de milliers de lignes.
// -----------------------------------------------------------------------------
class ApacheLogParser {
public:
    // Analyse une ligne. Le champ .valid indique si l'analyse a reussi.
    static LogEntry parseLine(const std::string& line);

private:
    // Convertit "06/Jul/2026:10:15:32 +0200" en "2026-07-06".
    static std::string normalizeDate(const std::string& apacheDate);
};
