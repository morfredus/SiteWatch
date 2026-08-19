/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
#include <vector>

#include "config/Config.h"
#include "coherence/CoherenceCheck.h"

class QTableWidget;
class QLabel;
class QPushButton;

// -----------------------------------------------------------------------------
// CoherenceDialog : panneau dédié « Contrôle de cohérence » (chantier B2).
//
// Confronte, par site et par fichier, les trois états d'un log - o2switch (source
// primaire), objet du collecteur SÉLECTIONNÉ, cache local - en métadonnées seules
// (aucun .gz téléchargé pour le seul contrôle). S'appuie sur coherence::checkSite.
//
// Aucune auto-réparation : le panneau détecte, explique, et propose des actions
// que l'utilisateur déclenche (remplir depuis le collecteur, télécharger en
// direct, relancer la collecte). Le collecteur reste un relais : il n'est jamais
// implicitement la source de vérité (o2switch l'est ; le cache est l'autorité
// fonctionnelle).
// -----------------------------------------------------------------------------
class CoherenceDialog : public QDialog {
    Q_OBJECT
public:
    // `readUrl` : collecteur choisi pour la lecture (celui qui alimente le cache).
    // `collectors` : tous les collecteurs vus (baseUrl -> app), pour information.
    CoherenceDialog(const Config& config, const QString& readUrl,
                    const QMap<QString, QString>& collectors, QWidget* parent = nullptr);

private:
    void runChecks();                       // relance le contrôle de tous les sites
    void populate();                        // (re)remplit le tableau depuis reports_
    const SiteConfig* siteByName(const std::string& name) const;

    // Actions par ligne (déclenchées par l'utilisateur), suivies d'un re-contrôle.
    void fillFromCollector(const std::string& site, const std::string& file,
                           const std::string& objectId);
    void downloadDirect(const std::string& site, const std::string& file);
    void collectNow(const std::string& site);

    Config                             config_;
    QString                            readUrl_;
    QMap<QString, QString>             collectors_;
    std::vector<coherence::SiteReport> reports_;

    QTableWidget* table_  = nullptr;
    QLabel*       header_ = nullptr;
    QLabel*       status_ = nullptr;
    QPushButton*  verifyBtn_ = nullptr;
};
