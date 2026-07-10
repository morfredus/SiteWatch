/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/Theme.h"

#include <QApplication>
#include <QStyleHints>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QPalette>
#include <QColor>

// Compatibilité Qt :
//  - Qt::ColorScheme, QStyleHints::colorScheme() / colorSchemeChanged : Qt 6.5+
//  - QStyleHints::setColorScheme()                                    : Qt 6.8+
// Avant Qt 6.5 (ex. Qt système sur certaines distributions Linux), on se replie
// sur la détection du thème via la palette de l'application.
#define SW_HAS_COLOR_SCHEME    (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
#define SW_HAS_SET_COLOR_SCHEME (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))

namespace {
const char* kSettingsKey = "appearance/themeMode";

// QSettings à identité explicite (organisation, application) : stocke la seule
// préférence de thème SANS dépendre de organizationName global — ce qui
// laisse QStandardPaths (donc le dossier de config.json) inchangé.
QSettings themeSettings() {
    return QSettings(QStringLiteral("morfredus"), QStringLiteral("SiteWatch"));
}

Theme::Mode modeFromString(const QString& s) {
    if (s == "light") return Theme::Mode::Light;
    if (s == "dark")  return Theme::Mode::Dark;
    return Theme::Mode::System;
}

QString modeToString(Theme::Mode m) {
    switch (m) {
        case Theme::Mode::Light: return "light";
        case Theme::Mode::Dark:  return "dark";
        default:                 return "system";
    }
}
} // namespace

Theme& Theme::instance() {
    static Theme theme;
    return theme;
}

Theme::Theme(QObject* parent) : QObject(parent) {
#if SW_HAS_COLOR_SCHEME
    // En mode « Système », suivre les changements de thème de l'OS à chaud.
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this](Qt::ColorScheme) {
                if (mode_ == Mode::System) reapply();
            });
#endif
    // Avant Qt 6.5, pas de signal de changement de thème : le thème « Système »
    // est déterminé au démarrage (et à chaque choix via le menu).
}

Theme::Mode Theme::savedMode() {
    return modeFromString(themeSettings().value(kSettingsKey, "system").toString());
}

void Theme::apply(Mode mode) {
    mode_ = mode;
    themeSettings().setValue(kSettingsKey, modeToString(mode));

#if SW_HAS_SET_COLOR_SCHEME
    // Force (ou libère) le schéma de couleurs des widgets natifs — menus,
    // dialogues standard, ascenseurs — pour qu'ils s'accordent au thème.
    // Unknown = « suivre le système » (mode Système).
    switch (mode) {
        case Mode::Light:  qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);   break;
        case Mode::Dark:   qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);    break;
        case Mode::System: qApp->styleHints()->setColorScheme(Qt::ColorScheme::Unknown); break;
    }
#endif
    // Avant Qt 6.8, setColorScheme n'existe pas : la feuille de style (QSS)
    // applique tout de même le thème choisi à l'ensemble de l'interface.
    reapply();
}

bool Theme::resolveDark() const {
    switch (mode_) {
        case Mode::Light: return false;
        case Mode::Dark:  return true;
        case Mode::System:
#if SW_HAS_COLOR_SCHEME
            return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
            // Repli Qt < 6.5 : un fond de fenêtre sombre indique un thème sombre.
            return qApp->palette().color(QPalette::Window).lightness() < 128;
#endif
    }
    return false;
}

QHash<QString, QString> Theme::loadPalette(bool dark) const {
    QHash<QString, QString> palette;
    QFile file(dark ? ":/themes/dark.theme" : ":/themes/light.theme");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return palette;

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;   // commentaire
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (!key.isEmpty() && !value.isEmpty()) palette.insert(key, value);
    }
    return palette;
}

void Theme::reapply() {
    dark_ = resolveDark();
    palette_ = loadPalette(dark_);

    QFile qssFile(":/themes/app.qss");
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString qss = QString::fromUtf8(qssFile.readAll());

    // Substitution des jetons @{nom} par la couleur du thème actif.
    for (auto it = palette_.constBegin(); it != palette_.constEnd(); ++it)
        qss.replace("@{" + it.key() + "}", it.value());

    qApp->setStyleSheet(qss);
    emit changed();
}

QString Theme::color(const QString& token) const {
    return palette_.value(token, QStringLiteral("#808080"));
}
