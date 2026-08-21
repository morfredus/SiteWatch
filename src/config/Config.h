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
    std::string id;            // UUID STABLE du site (identite pour morfCollector) ;
                               // genere au chargement s'il manque, jamais reutilise
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

// Un depot GitHub suivi pour ses metriques d'audience (distinct d'un site Web).
struct GitHubRepoConfig {
    std::string name;    // nom du depot, sans le proprietaire
    bool        enabled = true;
};

// Presence GitHub : SiteWatch en est proprietaire, morfCollector l'execute.
struct GitHubConfig {
    bool        enabled = false;
    std::string owner;           // compte GitHub, ex. "morfredus"
    std::string token;           // PAT local ; pousse au coffre, jamais au manifeste
    std::string dailyAt;         // "HH:MM" ; vide = heure globale du collecteur
    std::vector<GitHubRepoConfig> repositories;
};

// -----------------------------------------------------------------------------
// Config : configuration globale de l'application.
// -----------------------------------------------------------------------------
struct Config {
    std::string cacheRoot;             // ex. "D:/SiteWatch"
    std::string collectorUrl;          // URL de morfCollector (ex. "http://pi4dev:8792") ;
                                       // vide = découverte automatique via morfBeacon
    std::string collectorDailyAt;      // heure de collecte quotidienne "HH:MM" (défaut 02:00)
    std::string analyticsUrl;          // URL de morfAnalytics (ex. "http://pi4dev:8799") ;
                                       // vide = même hôte que le collecteur, sinon 1er vu
    std::vector<SiteConfig> sites;
    GitHubConfig github;

    // Charge la configuration depuis un fichier JSON.
    // Renvoie false et remplit 'error' si la lecture echoue.
    static bool load(const std::string& path, Config& out, std::string& error);

    // Écrit la configuration dans un fichier JSON (indenté).
    // Renvoie false et remplit 'error' si l'écriture echoue.
    static bool save(const std::string& path, const Config& config, std::string& error);
};
