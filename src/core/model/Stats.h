/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <array>
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

    // Provenance du trafic : moteurs/reseaux connus (Google, Bing...), "Direct",
    // sinon le nom de domaine du referent (ex. "exemple.com").
    std::map<std::string, long> referers;
    // Part de ces requetes qui ne sont que des ressources (image, CSS, JS...) et non des
    // pages : une signature d'e-mail chargee par un webmail y apparait, pas dans les visites.
    std::map<std::string, long> referersAssets;
    // Nature des ressources par referent : referent -> (image, style_script, font, icon,
    // media, other) -> nombre.
    std::map<std::string, std::map<std::string, long>> referersAssetKinds;
    // Requetes dont le User-Agent est celui d'un client ou d'un proxy de messagerie
    // (GoogleImageProxy, Outlook, Thunderbird...), par referent. Un indice, pas une preuve
    // d'ouverture de mail : un proxy peut prechager l'image sans que le message soit lu.
    std::map<std::string, long> referersMailHints;
    // Requetes de ressources faites par un client de messagerie, quel que soit le referent
    // (ils n'en envoient souvent aucun) : famille du client -> nombre.
    std::map<std::string, long> mailAgentAssets;

    // URL les plus attaquees (tentatives d'attaque + 404).
    std::map<std::string, long> topAttacked;

    // --- Series quotidiennes pour les graphiques ("AAAA-MM-JJ" -> nombre) ---
    std::map<std::string, long> dailyHumans;
    std::map<std::string, long> dailyBots;
    std::map<std::string, long> dailyNormal;  // activite WordPress legitime
    std::map<std::string, long> dailyAI;      // robots IA (Claude, OpenAI, Perplexity)
    std::map<std::string, long> dailySEO;     // robots SEO (Semrush, Ahrefs, MJ12, DotBot)
    std::map<std::string, long> daily404;     // erreurs 404
    std::map<std::string, long> daily403;     // acces refuses 403
    std::map<std::string, long> daily500;     // erreurs serveur 500
    std::map<std::string, long> dailyAttacks; // tentatives d'attaque

    // --- Analyses approfondies (publiees vers morfAnalytics) ---
    // Liens casses : URL en 404 qui ne sont PAS des scans (un scan est compte a part).
    std::map<std::string, long> topBroken;
    // Profil horaire (heure 0-23) sur toute la periode.
    std::array<long, 24> hourHumans{};
    std::array<long, 24> hourBots{};
    std::array<long, 24> hourAttacks{};
    // Pages vues par jour (humains, succes) et robots par jour : permettent de comparer
    // la derniere semaine a la precedente (pages en hausse ou en baisse, nouveaux robots).
    std::map<std::string, std::map<std::string, long>> dailyPages;
    std::map<std::string, std::map<std::string, long>> dailyBotEngines;
};
