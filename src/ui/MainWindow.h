/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QMainWindow>
#include <functional>
#include <vector>
#include "config/Config.h"
#include "core/model/Stats.h"
#include "core/model/LogEntry.h"
#include "ui/UrlReport.h"
#include "ui/Icons.h"

class QComboBox;
class QTabWidget;
class QTableWidget;
class QTreeWidget;
class QLabel;
class QWidget;
class QLineEdit;
class QDateEdit;
class QProgressBar;
class QVBoxLayout;
class QCheckBox;
class QFrame;
class QPushButton;
class QToolButton;
QT_BEGIN_NAMESPACE
class QChartView;
QT_END_NAMESPACE

// -----------------------------------------------------------------------------
// MainWindow : la seule classe qui connait Qt.
//
// Elle orchestre le coeur (Config -> CacheManager -> GzReader -> Parser ->
// StatsEngine) et affiche le resultat dans une interface facon Windows 11.
// -----------------------------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;  // double-clic santé

private slots:
    void onAnalyze();        // declenche l'analyse du site selectionne
    void onSync();           // telecharge les nouveaux logs distants (SFTP)
    void onOpenSettings();   // ouvre la fenetre de configuration
    void onSearch();         // recherche dans les entrees (IP/URL/robot/date/code)
    void onCleanCache();     // suppression de logs telecharges
    void onHelp();           // fenetre d'aide
    void onAbout();          // fenetre a propos
    void onPresetChanged();  // applique une periode predefinie
    void onChartChanged();   // change la courbe de l'onglet Graphiques

private:
    // Une carte KPI : on garde les deux libelles a mettre a jour.
    struct Kpi { QLabel* value = nullptr; QLabel* sub = nullptr; };

    // Autorise l'IP publique courante sur le pare-feu o2switch via l'API cPanel.
    // Renvoie false + message si l'appel échoue (sans jeton : ne fait rien -> true).
    bool whitelistSsh(const SiteConfig& site, QString& error);

    // Configuration du site actuellement sélectionné (ou nullptr).
    const SiteConfig* currentSite() const;
    // Variante modifiable (pour appliquer un filtre déduit puis sauvegarder).
    SiteConfig* currentSiteMutable();

    // --- Bannière intégrée (messages non bloquants remplaçant les MessageBox) ---
    enum class BannerLevel { Info, Success, Warning, Error };
    // Affiche la bannière. 'html' accepte du texte riche ; si 'actionText' est
    // non vide, un bouton d'action est affiché et déclenche 'onAction'.
    void showBanner(BannerLevel level, const QString& html,
                    const QString& actionText = {},
                    std::function<void()> onAction = {});
    void hideBanner();
    // Applique un filtre déduit au site courant, l'enregistre puis relance la
    // synchronisation (bouton « Utiliser ce filtre »).
    void applySuggestedFilter(const QString& filter);

    // Chemin du config.json (emplacement standard %LOCALAPPDATA%\SiteWatch).
    static QString configFilePath();
    void populateSiteSelector();

    void buildUi();
    void loadConfiguration();
    void displayStats(const Stats& stats);
    void refreshSitesOverview();
    void fillHealth(const Stats& stats);
    void fillUrls();         // remplit l'onglet URLs selon la catégorie choisie
    void fillRobotsTree(const Stats& stats);
    void fillTopRobots(const Stats& stats);
    void rebuildDonut(const Stats& stats);
    void rebuildChart();
    void showUrlDetail(const QString& url);   // fenêtre de détail d'une URL (double-clic)
    void navigateHealth(int index);           // saut vers l'onglet d'un indicateur santé

    // --- Onglets interactifs (double-clic = détail, clic droit = copier/exporter) ---
    // Nature du contenu d'une ligne, qui détermine comment retrouver les entrées.
    enum class DetailSource { Url, Referer, Attack, Normal };
    // Câble double-clic + menu contextuel sur un tableau d'onglet.
    void wireDetailTable(QTableWidget* table, DetailSource src);
    // Ouvre la fenêtre de détail pour un libellé de ligne.
    void openDetail(DetailSource src, const QString& label);
    // Entrées correspondant à un libellé (URL, référent, catégorie, activité).
    std::vector<const LogEntry*> matchesFor(DetailSource src, const QString& label) const;
    // Groupes {libellé, entrées} des lignes sélectionnées (pour l'export multiple).
    std::vector<urlreport::Group> groupsFor(QTableWidget* table, DetailSource src,
                                            const std::vector<int>& rows) const;

    QWidget* makeKpiCard(icons::Glyph glyph, const QString& title,
                         const QString& subRole, Kpi& out);

    Config  config_;
    QString configError_;
    Stats   lastStats_;
    std::vector<LogEntry> entries_;   // entrées de la dernière analyse (pour le détail)

    // --- En-tete ---
    QComboBox* siteSelector_ = nullptr;
    QLabel*    metaHeader_   = nullptr;   // ligne permanente site/periode/fichiers
    QDateEdit* fromDate_     = nullptr;
    QDateEdit* toDate_       = nullptr;
    QComboBox* presetCombo_  = nullptr;

    // --- Cartes KPI ---
    Kpi kTotal_, kHumans_, kRobots_, k404_, k403_, k500_;

    // --- Onglets ---
    QTabWidget*   tabs_          = nullptr;
    QWidget*      healthTab_     = nullptr;
    QWidget*      sitesTab_      = nullptr;
    QTableWidget* sitesTable_    = nullptr;
    QLabel*       sitesSummary_  = nullptr;
    QWidget*      previousDetailTab_ = nullptr;
    QLabel*       verdictLabel_  = nullptr;
    QVBoxLayout*  healthList_    = nullptr;
    QWidget*      robotsTab_     = nullptr;
    QTreeWidget*  robotsTree_    = nullptr;
    QWidget*      donutPanel_    = nullptr;
    QTableWidget* topRobotsTable_= nullptr;
    QTableWidget* securityTable_ = nullptr;
    QTableWidget* activityTable_ = nullptr;
    QTableWidget* pagesTable_    = nullptr;
    QTableWidget* referersTable_ = nullptr;
    QWidget*      urlsTab_      = nullptr;
    QTableWidget* attackedTable_ = nullptr;
    QComboBox*    urlFilter_    = nullptr;

    QWidget*    chartTab_      = nullptr;
    QComboBox*  chartSelector_ = nullptr;

    // --- Recherche ---
    QLineEdit*    searchEdit_  = nullptr;
    QTableWidget* searchTable_ = nullptr;
    QLabel*       searchInfo_  = nullptr;

    // --- Bannière intégrée ---
    QFrame*      banner_       = nullptr;
    QLabel*      bannerIcon_   = nullptr;
    QLabel*      bannerText_   = nullptr;
    QPushButton* bannerAction_ = nullptr;
    QToolButton* bannerClose_  = nullptr;
    std::function<void()> bannerActionFn_;

    // --- Barre de statut ---
    QProgressBar* progressBar_ = nullptr;
    QLabel*       statusRight_ = nullptr;
};
