/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/cache/CacheManager.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

CacheManager::CacheManager(std::string cacheRoot)
    : cacheRoot_(std::move(cacheRoot)) {}

std::string CacheManager::siteDir(const std::string& siteName) const {
    fs::path dir = fs::path(cacheRoot_) / siteName;
    std::error_code ec;
    fs::create_directories(dir, ec); // sans exception si le dossier existe deja
    return dir.string();
}

std::vector<std::string> CacheManager::localLogs(const std::string& siteName) const {
    std::vector<std::string> files;
    fs::path dir = fs::path(cacheRoot_) / siteName;

    std::error_code ec;
    if (!fs::exists(dir, ec)) return files;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".gz")
            files.push_back(entry.path().string());
    }

    // Tri alphabetique : donne un ordre stable (Jun avant Jul, etc.).
    std::sort(files.begin(), files.end());
    return files;
}
