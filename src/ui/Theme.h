/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QObject>
#include <QString>
#include <QHash>

// -----------------------------------------------------------------------------
// Theme : gestion centralisée de l'apparence.
//
// Charge la feuille de style maître (resources/themes/app.qss), y injecte les
// couleurs du thème choisi (light.theme / dark.theme), et applique le tout à
// l'application. En mode « Système », suit le thème clair/sombre de l'OS
// (Windows comme Linux) et réagit à ses changements.
//
// Les rares couleurs encore fixées côté C++ (séparateurs de donut, texte gris
// d'items de tableau, thème des graphiques) interrogent color()/isDark() pour
// rester cohérentes avec le thème actif.
// -----------------------------------------------------------------------------
class Theme : public QObject {
    Q_OBJECT
public:
    enum class Mode { System, Light, Dark };

    // Instance unique (créée au premier appel).
    static Theme& instance();

    // Applique un mode et le mémorise (QSettings). À appeler une fois au
    // démarrage, puis à chaque changement depuis le menu.
    void apply(Mode mode);

    Mode mode() const { return mode_; }

    // Mode restauré depuis les préférences (défaut : Système).
    static Mode savedMode();

    // true si le thème effectivement affiché est le sombre.
    bool isDark() const { return dark_; }

    // Couleur du thème actif pour un jeton donné (ex. "textMuted", "surface").
    // Renvoie une valeur de repli neutre si le jeton est inconnu.
    QString color(const QString& token) const;

signals:
    // Émis après chaque application de thème (initiale ou changement système).
    // Permet de re-teinter ce que le QSS ne couvre pas (graphiques…).
    void changed();

private:
    explicit Theme(QObject* parent = nullptr);
    void reapply();                       // recharge le QSS selon mode_ + OS
    bool resolveDark() const;             // sombre effectif pour le mode courant
    QHash<QString, QString> loadPalette(bool dark) const;

    Mode mode_ = Mode::System;
    bool dark_ = false;
    QHash<QString, QString> palette_;     // palette effective courante
};
