/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"
#include "ui/Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    // NB : on ne définit PAS organizationName ici. QStandardPaths s'en sert pour
    // construire le dossier de configuration (config.json) ; l'ajouter
    // déplacerait ce dossier et « perdrait » la config des installations
    // existantes. La préférence de thème utilise un QSettings à nom explicite
    // (voir Theme.cpp), sans toucher à cet emplacement.
    app.setApplicationName("SiteWatch");
    app.setWindowIcon(QIcon(":/app.ico"));

    // Apparence : la feuille de style est externalisée (resources/themes/).
    // Le thème (clair / sombre / système) est restauré depuis les préférences.
    Theme::instance().apply(Theme::savedMode());

    MainWindow window;
    window.show();
    return app.exec();
}
