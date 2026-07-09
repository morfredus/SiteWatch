/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// SiteConfig : parametres d'un site a analyser (lus depuis config.json).
// -----------------------------------------------------------------------------
struct SiteConfig {
    std::string name;          // ex. "morfredus"
    std::string host;          // ex. "morfredus.fr"
    std::string protocol;      // "sftp" ou "ftp"
    std::string user;
    std::string password;      // vide si connexion par cle
    std::string keyFile;       // chemin de la cle SSH (optionnel)
    std::string remoteLogDir;  // dossier des logs sur le serveur
    std::string cpanelToken;   // jeton d'API cPanel (o2switch : autorisation pare-feu SSH)
    std::string domain;        // domaine complet avec TLD (ex. "morfredus.fr") ; filtre le domaine principal, exclut les sous-domaines
    std::string logMatch;      // filtre AVANCE (optionnel) : si renseigne, un fichier est retenu si son nom contient ce motif ; sinon detection auto o2switch
};

// -----------------------------------------------------------------------------
// Config : configuration globale de l'application.
// -----------------------------------------------------------------------------
struct Config {
    std::string cacheRoot;             // ex. "D:/SiteWatch"
    std::vector<SiteConfig> sites;

    // Charge la configuration depuis un fichier JSON.
    // Renvoie false et remplit 'error' si la lecture echoue.
    static bool load(const std::string& path, Config& out, std::string& error);

    // Écrit la configuration dans un fichier JSON (indenté).
    // Renvoie false et remplit 'error' si l'écriture echoue.
    static bool save(const std::string& path, const Config& config, std::string& error);
};
