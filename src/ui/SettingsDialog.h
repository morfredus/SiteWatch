/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <vector>
#include <QString>
#include <QIcon>
#include "config/Config.h"

class QLineEdit;
class QListWidget;
class QToolButton;
class QLabel;
class QPushButton;
class QFormLayout;
class QComboBox;
class QVBoxLayout;
class QTimeEdit;
class QTableWidget;

// -----------------------------------------------------------------------------
// SettingsDialog : édition graphique de la configuration (config.json).
//
// L'utilisateur ne saisit que des informations fonctionnelles ; le préfixe des
// fichiers de logs est déduit automatiquement du « nom du site ».
// -----------------------------------------------------------------------------
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // `discoveredUrl` : URL du collecteur déjà connue (écouteur permanent de la
    // fenêtre principale). Le dialogue ne relance JAMAIS de découverte lui-même
    // (éviter de rebinder le port UDP 45454, ce qui casse l'écoute permanente).
    explicit SettingsDialog(const Config& config, const QString& discoveredUrl,
                            QWidget* parent = nullptr);

    // Configuration éditée (valide après acceptation).
    const Config& result() const { return config_; }

    // true si l'utilisateur a demandé l'envoi de la config à morfCollector
    // (bouton « Envoyer la configuration »). Lu par MainWindow après acceptation.
    bool pushRequested() const { return pushRequested_; }

private slots:
    void onSiteChanged(int row);
    void onAddSite();
    void onRemoveSite();
    void onBrowseCache();
    void onBrowseKey();
    void onTestConnection();
    void onAccept();
    void onNameEdited(const QString& text);

private:
    void buildUi();
    void buildCollectorGroup(QVBoxLayout* root);   // groupe morfCollector
    void refreshCollector();                        // se connecter : état + fichiers
    void refreshCollectorFiles();                   // fichiers du site sélectionné seulement
    void collectNowAll();                           // déclenche une collecte immédiate (toutes sources)
    void resetCollector();                          // efface les copies du Pi puis re-télécharge
    void pushAllCredentials(const QString& url);    // (re)dépose les secrets de tous les sites
    QString collectorUrl() const;                   // URL effective (champ, sinon découverte)
    void refreshSiteList();
    void loadSiteToForm(int index);
    void commitFormToSite(int index);
    void setFormEnabled(bool on);
    void showTestResult(int state, const QString& message);  // 1=OK, 2=échec, 0=à tester
    void applyResultStyle(int state);
    void updateSummary();
    QIcon stateIcon(int index) const;

    Config config_;
    QString discoveredUrl_;               // URL du collecteur fournie par MainWindow
    std::vector<int>     siteState_;      // 0 à tester, 1 valide, 2 erreur
    std::vector<QString> lastReport_;     // dernier rapport de test par site
    int  current_ = -1;
    bool loading_ = false;
    bool pushRequested_ = false;

    QLineEdit*   cacheEdit_   = nullptr;
    QListWidget* sitesList_   = nullptr;
    QLabel*      summaryLabel_ = nullptr;

    QLineEdit*   nameEdit_   = nullptr;
    QLineEdit*   hostEdit_   = nullptr;
    QLineEdit*   userEdit_   = nullptr;
    QLineEdit*   keyEdit_    = nullptr;
    QLineEdit*   passEdit_   = nullptr;
    QToolButton* passEye_    = nullptr;
    QLineEdit*   remoteEdit_ = nullptr;
    QLineEdit*   tokenEdit_  = nullptr;
    QToolButton* tokenEye_   = nullptr;
    QLineEdit*   logMatchEdit_ = nullptr;

    QPushButton* testButton_ = nullptr;
    QLabel*      testResult_ = nullptr;

    // --- morfCollector ---
    QLineEdit*   collectorEdit_  = nullptr;   // URL (vide = découverte)
    QTimeEdit*   collectorTime_  = nullptr;   // heure de collecte quotidienne
    QLabel*      collectorState_ = nullptr;   // état / config du collecteur (synthèse)
    QTableWidget* collectorSites_ = nullptr;  // sites confiés : état + nb de fichiers + taille
    QComboBox*   collectorSite_  = nullptr;   // site dont on liste les copies
    QListWidget* collectorFiles_ = nullptr;   // fichiers conservés (object_id en UserRole)
};
