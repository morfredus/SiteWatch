/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;
class QDateEdit;
class QListWidget;
class QLabel;
class QPushButton;

// -----------------------------------------------------------------------------
// CacheCleanupDialog : suppression des logs .gz téléchargés.
//
// Filtre par site (ou tous), par mode (tout / antérieurs à un mois / période),
// puis liste cochable pour affiner la sélection avant suppression.
// -----------------------------------------------------------------------------
class CacheCleanupDialog : public QDialog {
    Q_OBJECT
public:
    CacheCleanupDialog(const QString& cacheRoot, const QStringList& siteNames,
                       QWidget* parent = nullptr);

private:
    void buildUi();
    void updateMode();     // affiche/masque les champs de date selon le mode
    void refreshList();    // recalcule la liste des fichiers concernés
    void updateSummary();  // met à jour le compteur (fichiers cochés + taille)
    void onDelete();

    QString     cacheRoot_;
    QStringList siteNames_;

    QComboBox*   siteCombo_   = nullptr;
    QComboBox*   modeCombo_   = nullptr;
    QLabel*      dateFromLbl_ = nullptr;
    QDateEdit*   dateFrom_    = nullptr;
    QLabel*      dateToLbl_   = nullptr;
    QDateEdit*   dateTo_      = nullptr;
    QListWidget* fileList_    = nullptr;
    QLabel*      summary_     = nullptr;
    QPushButton* deleteBtn_   = nullptr;
};
