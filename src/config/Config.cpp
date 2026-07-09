/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "config/Config.h"
#include <nlohmann/json.hpp>
#include <fstream>

using nlohmann::json;

// Petit utilitaire : lit une chaine optionnelle depuis un objet JSON.
static std::string getStr(const json& j, const char* key) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return "";
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

    out.sites.clear();
    if (j.contains("sites") && j["sites"].is_array()) {
        for (const auto& s : j["sites"]) {
            SiteConfig site;
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
            if (!site.name.empty()) out.sites.push_back(site);
        }
    }

    return true;
}

bool Config::save(const std::string& path, const Config& config, std::string& error) {
    json j;
    j["cacheRoot"] = config.cacheRoot;

    json sites = json::array();
    for (const auto& s : config.sites) {
        json o;
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
