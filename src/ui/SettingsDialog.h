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

// -----------------------------------------------------------------------------
// SettingsDialog : édition graphique de la configuration (config.json).
//
// L'utilisateur ne saisit que des informations fonctionnelles ; le préfixe des
// fichiers de logs est déduit automatiquement du « nom du site ».
// -----------------------------------------------------------------------------
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const Config& config, QWidget* parent = nullptr);

    // Configuration éditée (valide après acceptation).
    const Config& result() const { return config_; }

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
    void refreshSiteList();
    void loadSiteToForm(int index);
    void commitFormToSite(int index);
    void setFormEnabled(bool on);
    void showTestResult(int state, const QString& message);  // 1=OK, 2=échec, 0=à tester
    void applyResultStyle(int state);
    void updateSummary();
    QIcon stateIcon(int index) const;

    Config config_;
    std::vector<int>     siteState_;      // 0 à tester, 1 valide, 2 erreur
    std::vector<QString> lastReport_;     // dernier rapport de test par site
    int  current_ = -1;
    bool loading_ = false;

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
};
