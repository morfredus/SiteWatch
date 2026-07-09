/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/analytics/BotDetector.h"
#include <algorithm>
#include <array>

namespace {

// Table de correspondance signature -> (nom precis, moteur).
// L'ORDRE COMPTE : les signatures les plus specifiques sont placees en
// premier (ex. "Googlebot-Image" avant "Googlebot").
struct BotSignature {
    const char* needle;   // fragment recherche dans le User-Agent (minuscules)
    const char* name;     // nom precis affiche
    const char* engine;   // regroupement pour le tableau de bord
};

constexpr std::array<BotSignature, 22> kSignatures = {{
    // --- Google ---
    {"googlebot-image", "Google Image", "Google"},
    {"googlebot-news",  "Google News",  "Google"},
    {"googlebot",       "Googlebot",    "Google"},
    {"google-inspectiontool", "Google Inspection", "Google"},
    {"adsbot-google",   "AdsBot Google", "Google"},
    // --- Bing / Microsoft ---
    {"bingbot",         "Bingbot",      "Bing"},
    {"msnbot",          "MSNBot",       "Bing"},
    // --- Apple ---
    {"applebot",        "Applebot",     "Apple"},
    // --- Facebook / Meta ---
    {"facebookexternalhit", "Facebook", "Facebook"},
    {"facebot",         "Facebook",     "Facebook"},
    {"meta-external",   "Meta",         "Facebook"},
    // --- IA ---
    {"claudebot",       "ClaudeBot",    "Claude"},
    {"claude-searchbot","Claude SearchBot", "Claude"},
    {"anthropic",       "Anthropic",    "Claude"},
    {"gptbot",          "GPTBot",       "OpenAI"},
    {"chatgpt",         "ChatGPT",      "OpenAI"},
    {"oai-searchbot",   "OAI SearchBot","OpenAI"},
    {"perplexity",      "PerplexityBot","Perplexity"},
    // --- SEO ---
    {"semrushbot",      "SemrushBot",   "Semrush"},
    {"ahrefsbot",       "AhrefsBot",    "Ahrefs"},
    {"mj12bot",         "MJ12bot",      "MJ12"},
    {"dotbot",          "DotBot",       "DotBot"},
}};

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

} // namespace

BotResult BotDetector::detect(const std::string& userAgent) {
    BotResult r;
    if (userAgent.empty() || userAgent == "-") return r; // pas d'UA -> traite ailleurs

    const std::string ua = toLower(userAgent);

    // 1. Recherche dans la table des robots connus.
    for (const auto& sig : kSignatures) {
        if (ua.find(sig.needle) != std::string::npos) {
            r.isBot  = true;
            r.name   = sig.name;
            r.engine = sig.engine;
            return r;
        }
    }

    // 2. Robot generique non liste mais qui se declare comme tel.
    if (ua.find("bot") != std::string::npos ||
        ua.find("crawler") != std::string::npos ||
        ua.find("spider") != std::string::npos) {
        r.isBot  = true;
        r.name   = "Robot inconnu";
        r.engine = "Autres";
        return r;
    }

    // 3. Sinon : considere comme humain.
    return r;
}
