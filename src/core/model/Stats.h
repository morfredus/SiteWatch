/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <map>

// -----------------------------------------------------------------------------
// Stats : resultat agrege de l'analyse d'un site sur une periode.
//
// Toutes les donnees affichees dans le tableau de bord sortent d'ici.
// Les std::map gardent les cles triees, ce qui simplifie l'affichage.
// -----------------------------------------------------------------------------
struct Stats {
    long totalRequests = 0;   // nombre total de lignes de log analysees
    long humans        = 0;   // visiteurs humains
    long bots          = 0;   // requetes de robots

    // Compteurs d'erreurs mis en avant dans le resume permanent.
    long errors404 = 0;
    long errors403 = 0;
    long errors500 = 0;

    // Bornes de la periode reellement couverte par les logs ("AAAA-MM-JJ").
    std::string firstDate;
    std::string lastDate;

    // Comptage par "moteur" / robot (Google, Bing, Claude, OpenAI, Semrush...).
    std::map<std::string, long> botCounts;

    // Comptage par code HTTP (200, 301, 404, 403, 500...).
    std::map<int, long> statusCounts;

    // Activite WordPress LEGITIME : admin-ajax, wp-json, wp-login, wp-admin.
    std::map<std::string, long> normalActivity;

    // Tentatives d'ATTAQUE : xmlrpc, phpMyAdmin, install.php, cgi-bin,
    // shell/eval, sondages de plugins/themes en erreur.
    std::map<std::string, long> attackActivity;

    // Pages les plus consultees (url -> nombre de vues humaines).
    std::map<std::string, long> topPages;

    // Provenance du trafic (Google, Bing, DuckDuckGo, Facebook, Direct...).
    std::map<std::string, long> referers;

    // URL les plus attaquees (tentatives d'attaque + 404).
    std::map<std::string, long> topAttacked;

    // --- Series quotidiennes pour les graphiques ("AAAA-MM-JJ" -> nombre) ---
    std::map<std::string, long> dailyHumans;
    std::map<std::string, long> dailyBots;
    std::map<std::string, long> dailyNormal;  // activite WordPress legitime
    std::map<std::string, long> dailyAI;      // robots IA (Claude, OpenAI, Perplexity)
    std::map<std::string, long> dailySEO;     // robots SEO (Semrush, Ahrefs, MJ12, DotBot)
    std::map<std::string, long> daily404;     // erreurs 404
    std::map<std::string, long> dailyAttacks; // tentatives d'attaque
};
