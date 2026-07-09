/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/io/GzReader.h"
#include <zlib.h>
#include <vector>

bool GzReader::readLines(const std::string& path,
                        const std::function<void(const std::string&)>& callback) {
    gzFile file = gzopen(path.c_str(), "rb");
    if (!file) return false;

    // Tampon de lecture. gzgets lit au plus size-1 caracteres ou jusqu'a '\n'.
    // Une ligne de log peut etre longue : on prevoit large et on recolle les
    // morceaux si une ligne depasse la taille du tampon.
    std::vector<char> buffer(64 * 1024);
    std::string current;

    while (gzgets(file, buffer.data(), static_cast<int>(buffer.size())) != nullptr) {
        current += buffer.data();
        // Si la ligne se termine par '\n', elle est complete.
        if (!current.empty() && current.back() == '\n') {
            current.pop_back();
            if (!current.empty() && current.back() == '\r') current.pop_back();
            callback(current);
            current.clear();
        }
        // Sinon on continue d'accumuler (ligne plus longue que le tampon).
    }

    // Derniere ligne sans saut de ligne final.
    if (!current.empty()) callback(current);

    gzclose(file);
    return true;
}
