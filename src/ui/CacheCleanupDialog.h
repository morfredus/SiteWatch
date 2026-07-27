/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

class QComboBox;
class QDateEdit;
class QListWidget;
class QLabel;
class QPushButton;
class QVBoxLayout;

// -----------------------------------------------------------------------------
// CacheCleanupDialog : suppression des logs .gz téléchargés.
//
// Filtre par site (ou tous), par mode (tout / antérieurs à un mois / période),
// puis liste cochable pour affiner la sélection avant suppression.
// -----------------------------------------------------------------------------
class CacheCleanupDialog : public QDialog {
    Q_OBJECT
public:
    // `collectorSites` : couples (nom, source_id) pour piloter morfCollector.
    // `collectorUrl` : URL du collecteur (vide = découverte au besoin).
    CacheCleanupDialog(const QString& cacheRoot, const QStringList& siteNames,
                       const QVector<QPair<QString, QString>>& collectorSites,
                       const QString& collectorUrl,
                       QWidget* parent = nullptr);

private:
    void buildUi();
    void updateMode();     // affiche/masque les champs de date selon le mode
    void refreshList();    // recalcule la liste des fichiers concernés
    void updateSummary();  // met à jour le compteur (fichiers cochés + taille)
    void onDelete();

    void buildCollectorSection(QVBoxLayout* root);  // gestion des copies morfCollector
    void refreshCollector();
    QString effectiveCollectorUrl();

    QString     cacheRoot_;
    QStringList siteNames_;
    QVector<QPair<QString, QString>> collectorSites_;
    QString     collectorUrl_;

    QComboBox*   siteCombo_   = nullptr;
    QComboBox*   modeCombo_   = nullptr;
    QLabel*      dateFromLbl_ = nullptr;
    QDateEdit*   dateFrom_    = nullptr;
    QLabel*      dateToLbl_   = nullptr;
    QDateEdit*   dateTo_      = nullptr;
    QListWidget* fileList_    = nullptr;
    QLabel*      summary_     = nullptr;
    QPushButton* deleteBtn_   = nullptr;

    // --- Section morfCollector ---
    QComboBox*   colSite_   = nullptr;
    QListWidget* colFiles_  = nullptr;
    QLabel*      colState_  = nullptr;
};
