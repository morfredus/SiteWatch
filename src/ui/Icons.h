/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QChar>

class QFont;
class QColor;
class QPixmap;
class QIcon;

// -----------------------------------------------------------------------------
// icons : pictogrammes d'interface rendus par une POLICE D'ICÔNES embarquée
// (resources/fonts/SiteWatchIcons.ttf, sous-ensemble de Font Awesome Free).
//
// Objectif : un rendu identique sur Windows, Linux et WSL, sans dépendre des
// polices emoji du système (souvent absentes sous Linux/WSL, et mal rendues en
// couleur par Qt 6.4). On remplace donc les emoji Unicode de l'UI par des
// glyphes de cette police, colorés via le thème (feuille de style ou couleur
// explicite selon le contexte).
// -----------------------------------------------------------------------------
namespace icons {

enum class Glyph {
    Chart,        // 📊 total / statistiques
    User,         // 👤 humains
    Robot,        // 🤖 robots
    Warn,         // ⚠️ avertissement (triangle)
    Shield,       // 🛡️ protection / 403
    Ban,          // ⛔ interdit / 500
    Trophy,       // 🏆
    CrossCircle,  // ❌ croix cerclée
    TrendUp,      // 📈 tendance
    Heart,        // ❤️
    Check,        // ✓ / ✅ (coche)
    Cross,        // ✗ / ✕ (croix)
    CircleCheck,  // ✅ succès (coche cerclée)
    CircleInfo,   // ℹ️ information
    Eye,          // 👁 afficher/masquer
    Dot,          // ● pastille d'état (colorée selon le contexte)
};

// Charge la police au premier appel et renvoie le nom de sa famille
// (« SiteWatch Icons »). Utilisable dans une feuille de style (font-family).
QString family();

// Police d'icônes à la taille demandée (en pixels).
QFont font(int px);

// Caractère (glyphe) correspondant à une icône, pour l'afficher dans un widget
// dont la police est déjà celle des icônes (QLabel/QToolButton stylé en QSS).
QChar ch(Glyph g);

// Fragment HTML pour insérer une icône colorée DANS du texte enrichi (QLabel
// en rich text) : applique la police d'icônes, la taille et la couleur.
QString span(Glyph g, const QString& colorHex, int px = 14);

// Rendu bitmap de l'icône (fond transparent), pour un QIcon d'élément de liste
// ou de tableau (QTableWidgetItem/QListWidgetItem n'acceptent pas de rich text).
QPixmap pixmap(Glyph g, const QColor& color, int px = 16);
QIcon   icon(Glyph g, const QColor& color, int px = 16);

} // namespace icons
