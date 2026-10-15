/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <functional>
#include <atomic>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include "collector/CollectorSync.h"
#include "config/Config.h"
#include <functional>
#include <memory>
#include <vector>
#include "config/Config.h"
#include "core/model/Stats.h"
#include "webstore/WebAuthorityStore.h"
#include <memory>
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
class QStackedWidget;
class GitHubPage;
QT_BEGIN_NAMESPACE
class QChartView;
QT_END_NAMESPACE

namespace morfbeacon { class PresenceService; class IMetricsProvider; }

// -----------------------------------------------------------------------------
// MainWindow : la seule classe qui connait Qt.
//
// Elle orchestre le coeur (Config -> CacheManager -> GzReader -> Parser ->
// StatsEngine) et affiche le resultat dans une interface facon Windows 11.
// -----------------------------------------------------------------------------
// Travail de synchronisation avec les collecteurs, calcule dans un fil de travail : le fil de travail
// n'a acces qu'a des COPIES (jamais a config_ ni aux widgets).
struct CollectorWork {
    Config cfg;
    QString cfgPath;
    QStringList all;                 // collecteurs a qui pousser
    QString pinnedRead;              // source de lecture souhaitee
    bool force = false;              // remettre TOUS les secrets (« Tout synchroniser »)
    bool withGithub = false;         // lire aussi les instantanes GitHub (rattrapage d'ouverture)
    collectorsync::SyncResult r;
    QStringList added, removed, recovered, problems;
    QString readUrl;
    bool readFallback = false;
    int totalDl = 0, totalSkip = 0;
    QVector<QJsonObject> githubSnapshots;
};
struct SftpOutcome;                  // resultat d'un telechargement SFTP (defini dans le .cpp)

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;  // double-clic santé

private slots:
    void onAnalyze();        // declenche l'analyse du site selectionne
    void onSync();           // telecharge les nouveaux logs distants (SFTP)
    void onOpenSettings();   // ouvre la fenetre de configuration
    void onSearch();         // recherche dans les entrees (IP/URL/robot/date/code)
    void onCleanCache();     // suppression de logs telecharges
    void onCoherenceCheck(); // panneau de controle de coherence (B2)
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
    void checkForUpdates(bool manual);   // manual=true : entrée de menu ; false : au démarrage (silencieux)

    // --- Intégration morfCollector (côté fournisseur, contrat morfcollect/1) ---
    // Localise morfCollector (découverte morfBeacon), en mémorisant l'URL. "" si
    // aucun collecteur n'est présent (SiteWatch reste autonome).
    QString ensureCollector(int discoverTimeoutMs);
    QString ensureAnalytics();
    QStringList knownCollectorUrls() const;   // tous les collecteurs a qui pousser
    void applyGithubServices();
    void pinCollectorUrl(const QString& url);
    void pinAnalyticsUrl(const QString& url);
    void pushGithubConfig();
    // Au démarrage : pousse la config au collecteur si présent, et ALERTE sur les
    // sites ajoutés / retirés (un retrait n'efface JAMAIS les copies du Pi).
    void startupCollectorSync();
    void consolidateStaleSites();   // remplit le magasin web des sites au cache modifie (arriere-plan)

    // --- Travail en arriere-plan : l'interface ne doit jamais sembler figee ---
    // `work` tourne dans un fil du pool (il ne touche NI aux widgets NI aux bases SQLite) ;
    // `done` est rappele dans le fil de l'interface avec le resultat.
    void runInBackground(std::function<void()> work, std::function<void()> done);
    // Synchronisation avec les collecteurs (rattrapage d'ouverture, « Tout synchroniser »).
    void runCollectorWork(std::shared_ptr<CollectorWork> w, const QString& text, std::function<void()> done);
    // Telechargement SFTP d'un site : connexion, liste et copie dans un fil de travail, bannieres et
    // analyse dans l'interface. `done` est rappele quoi qu'il arrive (enchainement site par site).
    void startSftpSync(const SiteConfig& site, bool analyzeAfter, std::function<void()> done);
    void sftpWork(SftpOutcome& out, const SiteConfig& site, const std::string& cacheRoot);
    void postProgress(int percent);
    // Meme principe mais la fonction ne rend la main qu'a la fin (enchainements sequentiels) ;
    // la fenetre se redessine pendant l'attente, sans accepter de clic.
    void runPumped(const QString& text, const std::function<void()>& work);
    void postStatus(const QString& text);   // message de barre d'etat depuis un fil de travail
    void beginBusy(const QString& text);    // indicateur d'activite (barre de progression)
    void endBusy();
    void postReport(const QJsonObject& body);   // POST asynchrone de la synthese vers morfAnalytics
    void flushPendingReports();                 // renvoie les syntheses non publiees (morfAnalytics enfin joignable)
    void publishAnalytics();
    // « Tout synchroniser » : en une passe, tous les sites.
    void syncAllViaCollector();   // récupère les copies locales depuis le collecteur
    void syncAllLocal();          // télécharge en direct (SFTP), site par site

    // Bascule sur l'onglet portant ce libellé (navigation depuis les menus).
    void goToTab(const QString& tabText);

    // Onglet « Copies locales » : consulter/piloter les archives du collecteur.
    void buildCopiesTab();
    void refreshCopies();         // recharge collecteur + sources
    void loadCopiesObjects();     // recharge les objets de la source sélectionnée
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
    QString collectorUrl_;   // morfCollector de LECTURE découvert (1er vu ; vide si absent)
    QMap<QString, QString> collectorSeen_;  // baseUrl -> "app (hote)" : TOUS les collecteurs vus
    QMap<QString, QString> analyticsSeen_;  // baseUrl -> "app (hote)" : morfAnalytics vus
    bool collectorStartupDone_ = false;   // rattrapage d'ouverture déjà joué
    std::atomic<bool> cancel_{false};     // arret demande (fermeture) : les fils de travail s'interrompent
    bool syncBusy_ = false;            // rattrapage ou consolidation en arriere-plan en cours
    bool startupRerun_ = false;           // une demande est arrivee pendant ce temps : a rejouer ensuite
    QString busyText_;                    // libelle du travail en cours (declare en activity/1)
    qint64  busySince_ = 0;               // debut (epoch s) du travail en cours
    int  busyCount_ = 0;                  // taches d'arriere-plan actives (indicateur d'activite)
    QLabel* busyLabel_ = nullptr;         // indicateur tournant de la barre d'etat
    QTimer* busyTimer_ = nullptr;
    int  busyFrame_ = 0;
    QMap<QString, QJsonObject> pendingReports_;   // site_id -> derniere synthese non publiee
    int  overviewGen_ = 0;                // generation du tableau de synthese (ecarte un resultat perime)
    QString analyticsLive_;          // morfAnalytics retenu (epingle s'il repond, sinon un autre vu)
    QString analyticsCheckedFor_;    // adresse epinglee pour laquelle analyticsLive_ a ete calcule
    qint64  analyticsCheckedAt_ = 0; // epoch du dernier controle (cache court : pas de sonde a chaque appel)
    QString analyticsUrl_;   // dernier vu (repli si pas d'épingle ni même hôte)
    QPushButton* analyticsButton_ = nullptr;
    QStackedWidget* domainStack_ = nullptr;
    QPushButton* sitesDomainBtn_ = nullptr;
    QPushButton* githubDomainBtn_ = nullptr;
    GitHubPage*  githubPage_ = nullptr;
    Stats   lastStats_;
    std::unique_ptr<WebAuthorityStore> webStore_;   // verite web, ouverte a la demande
    WebAuthorityStore* webStore();
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

    // --- Onglet « Copies locales » (morfCollector) ---
    QWidget*      copiesTab_          = nullptr;
    QLabel*       copiesStatus_       = nullptr;
    QTableWidget* copiesSourcesTable_ = nullptr;
    QTableWidget* copiesObjectsTable_ = nullptr;

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

    // --- Supervision LAN (morfBeacon) ---
    // Fournisseur de metriques (non-QObject) detruit apres 'presence_', qui est
    // supprime explicitement dans le destructeur pour arreter proprement les
    // sockets avant que le provider ne disparaisse.
    std::unique_ptr<morfbeacon::IMetricsProvider> presenceMetrics_;
    morfbeacon::PresenceService*                  presence_ = nullptr;
};
