/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include "core/model/LogEntry.h"
#include "core/model/Stats.h"
#include <string>

// -----------------------------------------------------------------------------
// StatsEngine : accumule les LogEntry et produit un objet Stats.
//
// Utilisation :
//     StatsEngine engine;
//     for (chaque ligne)  engine.ingest(entry);
//     Stats resultat = engine.result();
// -----------------------------------------------------------------------------
class StatsEngine {
public:
    // Integre une ligne analysee dans les statistiques.
    void ingest(const LogEntry& entry);

    // Renvoie le resultat agrege.
    const Stats& result() const { return stats_; }

    // --- Classifieurs exposes a l'interface -------------------------------
    // Ils sont publics et statiques (fonctions pures) afin que l'interface
    // puisse retrouver, au double-clic sur une ligne, les entrees d'une
    // categorie d'attaque, d'une activite WordPress ou d'un referent - sans
    // dupliquer la logique de classification (source unique de verite).

    // Resultat de la classification d'une URL.
    enum class ActivityType { None, Normal, Attack };
    struct ActivityInfo {
        ActivityType type = ActivityType::None;
        std::string  label;   // ex. "XML-RPC", "wp-login", "phpMyAdmin"
    };

    // Classe une requete : activite WordPress legitime, tentative d'attaque,
    // ou rien de particulier. Le code HTTP sert a eviter les faux positifs
    // (un asset de plugin en 200 est normal ; en 404 c'est un sondage).
    static ActivityInfo classifyActivity(const LogEntry& entry);

    // Extrait le moteur de provenance depuis un referer.
    static std::string classifyReferer(const std::string& referer);

    // Type de provenance d'un libelle de classifyReferer : "direct", "search" (moteur de
    // recherche), "social" (reseau social), "mail" (webmail / messagerie) ou "site" (autre site).
    static std::string refererKind(const std::string& label);

    // Nature d'une ressource technique : "image", "style_script", "font", "icon", "media"
    // ou "other" ; chaine vide si l'URL n'est pas une ressource.
    static std::string assetKind(const std::string& url);

    // Famille du client de messagerie d'apres le User-Agent ("Gmail (proxy d'images)",
    // "Outlook", "Thunderbird"...) ; chaine vide si ce n'en est pas un.
    static std::string mailAgent(const std::string& userAgent);

    // Confiance que ce referent vient d'un message : "high" (referent de messagerie ET
    // User-Agent de client de messagerie), "medium" (l'un des deux), "" (aucun indice).
    // Jamais une preuve d'ouverture : un referent est declare par le client, un proxy peut
    // precharger une image.
    static std::string mailConfidence(const Stats& stats, const std::string& label);

private:
    // Regroupe les moteurs IA / SEO pour les series quotidiennes.
    static bool isAiEngine(const std::string& engine);
    static bool isSeoEngine(const std::string& engine);

    Stats stats_;
};
