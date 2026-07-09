/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <functional>

// -----------------------------------------------------------------------------
// GzReader : lit un fichier de log compresse (.gz) ligne par ligne.
//
// La lecture se fait en streaming (une ligne a la fois) pour ne jamais
// charger un fichier entier en memoire : indispensable sur de gros logs.
// -----------------------------------------------------------------------------
class GzReader {
public:
    // Ouvre le fichier .gz et appelle 'callback' pour chaque ligne.
    // Renvoie false si le fichier n'a pas pu etre ouvert.
    static bool readLines(const std::string& path,
                          const std::function<void(const std::string&)>& callback);
};
