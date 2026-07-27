/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "config/Config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <cstdio>

using nlohmann::json;

// Petit utilitaire : lit une chaine optionnelle depuis un objet JSON.
static std::string getStr(const json& j, const char* key) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return "";
}

// Genere un UUID v4 (sans dependance : utilise pour l'identite stable d'un site
// vis-a-vis de morfCollector -- l'identite ne doit jamais deriver du nom, cf.
// contrat morfcollect/1 §1.3). Genere une seule fois, puis persiste.
static std::string makeUuidV4() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd() ^ (static_cast<uint64_t>(rd()) << 32));
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t hi = dist(gen), lo = dist(gen);
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;  // version 4
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;  // variant 1
    char buf[37];
    std::snprintf(buf, sizeof(buf),
        "%08x-%04x-%04x-%04x-%012llx",
        static_cast<unsigned>(hi >> 32),
        static_cast<unsigned>((hi >> 16) & 0xFFFF),
        static_cast<unsigned>(hi & 0xFFFF),
        static_cast<unsigned>(lo >> 48),
        static_cast<unsigned long long>(lo & 0xFFFFFFFFFFFFULL));
    return std::string(buf);
}

bool Config::load(const std::string& path, Config& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Impossible d'ouvrir le fichier de configuration : " + path;
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        error = std::string("JSON invalide : ") + e.what();
        return false;
    }

    out.cacheRoot = getStr(j, "cacheRoot");
    if (out.cacheRoot.empty()) {
        error = "Le champ 'cacheRoot' est manquant dans config.json";
        return false;
    }
    out.collectorUrl     = getStr(j, "collectorUrl");     // facultatif (découverte sinon)
    out.collectorDailyAt = getStr(j, "collectorDailyAt"); // facultatif (défaut 02:00)

    out.sites.clear();
    if (j.contains("sites") && j["sites"].is_array()) {
        for (const auto& s : j["sites"]) {
            SiteConfig site;
            site.id           = getStr(s, "id");
            site.name         = getStr(s, "name");
            site.host         = getStr(s, "host");
            site.protocol     = getStr(s, "protocol");
            site.user         = getStr(s, "user");
            site.password     = getStr(s, "password");
            site.keyFile      = getStr(s, "keyFile");
            site.remoteLogDir = getStr(s, "remoteLogDir");
            site.cpanelToken  = getStr(s, "cpanelToken");
            site.domain       = getStr(s, "domain");
            site.logMatch     = getStr(s, "logMatch");
            // Migration : l'ancien champ "domain" (ex. morfredus.fr) devient le
            // "nom du site". Le préfixe de filtrage en est déduit automatiquement.
            if (!site.domain.empty()) site.name = site.domain;
            // Identite stable : attribuee une seule fois, puis conservee. Un site
            // sans id (config anterieure) en recoit un ici ; il sera persiste au
            // prochain save().
            if (site.id.empty()) site.id = makeUuidV4();
            if (!site.name.empty()) out.sites.push_back(site);
        }
    }

    return true;
}

bool Config::save(const std::string& path, const Config& config, std::string& error) {
    json j;
    j["cacheRoot"] = config.cacheRoot;
    if (!config.collectorUrl.empty())     j["collectorUrl"]     = config.collectorUrl;
    if (!config.collectorDailyAt.empty()) j["collectorDailyAt"] = config.collectorDailyAt;

    json sites = json::array();
    for (const auto& s : config.sites) {
        json o;
        o["id"]           = s.id.empty() ? makeUuidV4() : s.id;
        o["name"]         = s.name;
        o["host"]         = s.host;
        o["protocol"]     = s.protocol.empty() ? std::string("sftp") : s.protocol;
        o["user"]         = s.user;
        o["password"]     = s.password;
        o["keyFile"]      = s.keyFile;
        o["remoteLogDir"] = s.remoteLogDir;
        o["cpanelToken"]  = s.cpanelToken;
        if (!s.logMatch.empty()) o["logMatch"] = s.logMatch;
        sites.push_back(o);
    }
    j["sites"] = sites;

    std::ofstream out(path);
    if (!out) {
        error = "Écriture impossible : " + path;
        return false;
    }
    out << j.dump(2) << std::endl;
    if (!out.good()) {
        error = "Erreur lors de l'écriture de " + path;
        return false;
    }
    return true;
}
