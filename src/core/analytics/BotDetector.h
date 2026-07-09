/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>

// -----------------------------------------------------------------------------
// BotDetector : identifie un robot a partir de sa signature (User-Agent).
//
// Renvoie a la fois le nom precis du robot (ex. "Googlebot-Image") et le
// "moteur" utilise pour le tableau de bord (ex. "Google").
// -----------------------------------------------------------------------------
struct BotResult {
    bool        isBot = false;  // true si c'est un robot connu
    std::string name;           // nom precis, ex. "Bingbot", "GPTBot"
    std::string engine;         // regroupement dashboard, ex. "Bing", "OpenAI"
};

class BotDetector {
public:
    // Analyse un User-Agent. Si aucun robot connu n'est reconnu, isBot=false
    // et le visiteur est considere comme humain par le moteur de stats.
    static BotResult detect(const std::string& userAgent);
};
