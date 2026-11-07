/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

// Auto-test de Config : aller-retour de l'empreinte SSH, ecriture atomique (aucun
// fichier temporaire residuel) et droits 0600 sous Unix. Console pure, sans Qt.

#include "config/Config.h"
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("ECHEC ligne %d : %s\n", __LINE__, #cond); ++failures; } } while (0)

int main() {
    const fs::path dir = fs::temp_directory_path() / "sitewatch-config-selftest";
    fs::create_directories(dir);
    const std::string path = (dir / "config.json").string();

    Config c;
    c.cacheRoot = "cache";
    SiteConfig s;
    s.name = "exemple.fr";
    s.host = "exemple.fr";
    s.password = "secret";
    s.hostKeySha256 = "sha256:abcd";
    c.sites.push_back(s);

    std::string err;
    CHECK(Config::save(path, c, err));
    CHECK(!fs::exists(path + ".tmp"));   // le temporaire a ete renomme

    Config back;
    CHECK(Config::load(path, back, err));
    CHECK(back.sites.size() == 1);
    CHECK(back.sites[0].hostKeySha256 == "sha256:abcd");
    CHECK(back.sites[0].password == "secret");

    // Reecriture par-dessus un fichier existant (le renommage doit remplacer).
    c.sites[0].hostKeySha256.clear();
    CHECK(Config::save(path, c, err));
    CHECK(Config::load(path, back, err));
    CHECK(back.sites[0].hostKeySha256.empty());

#ifndef _WIN32
    const auto perms = fs::status(path).permissions();
    CHECK((perms & (fs::perms::group_all | fs::perms::others_all)) == fs::perms::none);
#endif

    std::error_code ec;
    fs::remove_all(dir, ec);
    std::printf(failures ? "config selftest : %d echec(s)\n" : "config selftest : OK\n", failures);
    return failures ? 1 : 0;
}
