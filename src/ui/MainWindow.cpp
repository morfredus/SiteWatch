/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/MainWindow.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>
#include <QStyle>
#include <QCheckBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QPainter>
#include <QPen>
#include <QMargins>
#include <QLocale>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QStatusBar>
#include <QProgressBar>
#include <QEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <vector>
#include <set>
#include <cctype>
#include <filesystem>

#include "core/cache/CacheManager.h"
#include "core/io/GzReader.h"
#include "core/parser/ApacheLogParser.h"
#include "core/analytics/StatsEngine.h"
#include "core/analytics/BotDetector.h"
#include "core/net/SftpClient.h"
#include "core/net/LogDiscovery.h"
#include "ui/SettingsDialog.h"
#include "ui/DetailDialog.h"
#include "ui/CacheCleanupDialog.h"
#include "ui/UrlReport.h"
#include "ui/Theme.h"
#include "ui/Icons.h"
#include <QClipboard>
#include <QStringList>

#ifndef SITEWATCH_VERSION
#define SITEWATCH_VERSION "dev"
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Constantes et helpers locaux
// ---------------------------------------------------------------------------
namespace {

struct RobotCategory {
    QString name;
    QString color;               // couleur du donut / de la legende
    std::vector<QString> engines;
};

const std::vector<RobotCategory> kRobotCategories = {
    {"IA",                   "#7c3aed", {"Claude", "OpenAI", "Perplexity"}},
    {"Moteurs de recherche", "#2563eb", {"Google", "Bing", "Apple"}},
    {"SEO",                  "#16a34a", {"Semrush", "Ahrefs", "MJ12", "DotBot"}},
    {"Divers",               "#9ca3af", {"Facebook", "Autres"}},
};

class SortItem : public QTableWidgetItem {
public:
    explicit SortItem(const QString& text) : QTableWidgetItem(text) {}

    bool operator<(const QTableWidgetItem& other) const override {
        const QVariant left = data(Qt::UserRole);
        const QVariant right = other.data(Qt::UserRole);
        if (left.isValid() && right.isValid()) {
            bool okL = false, okR = false;
            const double l = left.toDouble(&okL);
            const double r = right.toDouble(&okR);
            if (okL && okR) return l < r;
            return left.toString() < right.toString();
        }
        return text() < other.text();
    }
};

// Formatage des entiers avec separateur de milliers a la francaise (96 840).
QString num(long n) {
    return QLocale(QLocale::French, QLocale::France).toString(static_cast<qlonglong>(n));
}

QString pct(long value, long total) {
    if (total <= 0) return "0,0 %";
    double p = 100.0 * static_cast<double>(value) / static_cast<double>(total);
    return QString::number(p, 'f', 1).replace('.', ',') + " %";
}

QString humanSize(unsigned long long bytes) {
    double mo = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mo >= 1.0) return QString::number(mo, 'f', 1).replace('.', ',') + " Mo";
    double ko = static_cast<double>(bytes) / 1024.0;
    return QString::number(ko, 'f', 1).replace('.', ',') + " Ko";
}

long sumAttacks(const Stats& s) {
    long total = 0;
    for (const auto& [label, count] : s.attackActivity) total += count;
    return total;
}

long botOf(const Stats& s, const char* engine) {
    auto it = s.botCounts.find(engine);
    return it != s.botCounts.end() ? it->second : 0L;
}

long aiBots(const Stats& s) {
    return botOf(s, "Claude") + botOf(s, "OpenAI") + botOf(s, "Perplexity");
}

long normalActivityTotal(const Stats& s) {
    long total = 0;
    for (const auto& [label, count] : s.normalActivity) total += count;
    return total;
}

double ratioOf(long value, long total) {
    return total > 0 ? 100.0 * static_cast<double>(value) / static_cast<double>(total) : 0.0;
}

int healthLevel(const Stats& s) {
    const long total = s.totalRequests;
    const double attackRatio = ratioOf(sumAttacks(s), total);
    const double error404Ratio = ratioOf(s.errors404, total);

    int level = 0;
    level = std::max(level, s.errors500 == 0 ? 0 : (s.errors500 <= 10 ? 1 : 2));
    level = std::max(level, attackRatio < 1.0 ? 0 : (attackRatio < 5.0 ? 1 : 2));
    level = std::max(level, error404Ratio < 2.0 ? 0 : (error404Ratio < 10.0 ? 1 : 2));
    return level;
}

// Couleur (thème) d'un niveau de gravité : 0 = ok, 1 = avertissement, 2 = danger.
QString stateHex(int level) {
    return Theme::instance().color(level >= 2 ? "danger" : level == 1 ? "warn" : "ok");
}
QColor stateColor(int level) { return QColor(stateHex(level)); }

QString healthLabel(int level) {
    if (level >= 2) return "Intervention recommandée";
    if (level == 1) return "À surveiller";
    return "Normal";
}

QString attentionSummary(const Stats& s) {
    QStringList points;
    const long attacks = sumAttacks(s);
    auto attack = [&](const char* label) {
        auto it = s.attackActivity.find(label);
        return it != s.attackActivity.end() ? it->second : 0L;
    };
    const long xmlRpcCount = attack("XML-RPC");
    const long ai = aiBots(s);
    const long google = botOf(s, "Google");
    const long wp = normalActivityTotal(s);

    if (s.errors500 > 0) points << QString("%1 erreur(s) 500").arg(num(s.errors500));
    if (xmlRpcCount > 0) points << QString("Activité XML-RPC (%1)").arg(num(xmlRpcCount));
    else if (attacks > 0) points << QString("%1 tentative(s) d'attaque").arg(num(attacks));
    if (s.errors404 > 0) points << QString("%1 erreur(s) 404").arg(num(s.errors404));
    if (wp > 0 && ratioOf(wp, s.totalRequests) >= 25.0)
        points << "Activité WordPress élevée";
    if (ai > 0) points << QString("%1 robot(s) IA").arg(num(ai));
    if (s.totalRequests > 0 && google == 0) points << "Google peu actif";

    return points.isEmpty() ? "Rien à signaler" : points.mid(0, 3).join(" · ");
}

QString recommendedAction(const Stats& s) {
    const long attacks = sumAttacks(s);
    const long ai = aiBots(s);
    const long google = botOf(s, "Google");
    const long wp = normalActivityTotal(s);

    if (s.errors500 > 0) return "Analyser les erreurs serveur";
    if (attacks > 0) return "Examiner les attaques";
    if (s.errors404 > 0) return "Vérifier les erreurs 404";
    if (wp > 0 && ratioOf(wp, s.totalRequests) >= 25.0) return "Contrôler les plugins WordPress";
    if (s.totalRequests > 0 && google == 0) return "Vérifier le référencement";
    if (ai > 0) return "Surveiller les robots IA";
    return "Rien à signaler";
}

QTableWidgetItem* textItem(const QString& text, const QVariant& sortValue = {}) {
    auto* item = new SortItem(text);
    if (sortValue.isValid()) item->setData(Qt::UserRole, sortValue);
    return item;
}

QTableWidgetItem* numberItem(long value) {
    auto* item = new SortItem(num(value));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    item->setData(Qt::UserRole, static_cast<qlonglong>(value));
    return item;
}

QTableWidget* makeTable(const QString& col1, const QString& col2) {
    auto* t = new QTableWidget;
    t->setColumnCount(2);
    t->setHorizontalHeaderLabels({col1, col2});
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->verticalHeader()->setVisible(false);
    t->setShowGrid(false);
    return t;
}

void setRows(QTableWidget* t, const std::vector<std::pair<QString, long>>& rows) {
    t->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        t->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        auto* v = new QTableWidgetItem(num(rows[i].second));
        v->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        t->setItem(i, 1, v);
    }
}

// Variante a 3 colonnes : libelle / valeur / pourcentage du total.
void setRows3(QTableWidget* t, const std::vector<std::pair<QString, long>>& rows,
              long total) {
    t->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        t->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        auto* v = new QTableWidgetItem(num(rows[i].second));
        v->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        t->setItem(i, 1, v);
        auto* p = new QTableWidgetItem(pct(rows[i].second, total));
        p->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        p->setForeground(QColor(Theme::instance().color("textMuted")));
        t->setItem(i, 2, p);
    }
}

QTableWidget* makeTable3(const QString& c1, const QString& c2, const QString& c3) {
    auto* t = new QTableWidget;
    t->setColumnCount(3);
    t->setHorizontalHeaderLabels({c1, c2, c3});
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->verticalHeader()->setVisible(false);
    t->setShowGrid(false);
    return t;
}

// Gravite d'une categorie de l'onglet Securite (true = rouge, false = orange).
bool isSevereLabel(const QString& label) {
    static const QStringList rouge = {
        "XML-RPC", "shells / eval", "phpMyAdmin", "install.php",
        "CGI-BIN", "backup / dump", "Erreurs 500"};
    return rouge.contains(label);
}

std::vector<std::pair<QString, long>> sortedByValue(
        const std::map<std::string, long>& data, int maxRows = 0) {
    std::vector<std::pair<QString, long>> rows;
    rows.reserve(data.size());
    for (const auto& [k, v] : data)
        rows.emplace_back(QString::fromStdString(k), v);
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (maxRows > 0 && static_cast<int>(rows.size()) > maxRows)
        rows.resize(maxRows);
    return rows;
}

// Un fichier de log o2switch du domaine principal est nommé
// "<domaine sans les points>.bifr7024.odns.fr-<Mois>-<Année>.gz"
// (ex. morfredus.fr -> "morfredusfr.bifr7024.odns.fr-Jul-2026.gz").
// On compare donc le PRÉFIXE du nom (avant le 1er point) au domaine de la
// config privé de ses points. Cela exclut les sous-domaines (dev., test.,
// sarah., stats.…) et fonctionne pour tout TLD (.fr, .com, etc.).
std::string lowerStr(const std::string& s) {
    std::string r; r.reserve(s.size());
    for (unsigned char c : s) r += static_cast<char>(std::tolower(c));
    return r;
}

bool matchesAny(const std::string& lurl, std::initializer_list<const char*> pats) {
    for (const char* p : pats)
        if (lurl.find(p) != std::string::npos) return true;
    return false;
}

// Fonctionnement interne de WordPress (dual-usage), pas une vraie attaque.
bool isWordPressInternal(const std::string& url) {
    return matchesAny(lowerStr(url), {
        "xmlrpc.php", "wp-cron.php", "admin-ajax.php", "wp-login",
        "/wp-json", "/wp-admin", "/wp-includes/", "wp-content", "wp-embed", "wp-trackback"});
}

// Scan/attaque externe probable (aucune raison légitime sur un site WordPress).
bool isProbableAttack(const std::string& url) {
    return matchesAny(lowerStr(url), {
        "phpmyadmin", "/pma", "/mysql", "install.php", "cgi-bin", "shell",
        "eval(", ".php7", "wp-config", "/.env", "author=", "/backup", ".sql",
        ".bak", "/old/", "/wordpress/", "/.git", "/vendor/", "/.aws", "/config"});
}

// Requête technique/système (robots, sitemap, favicon, .well-known…).
bool isSystemRequest(const std::string& url) {
    return matchesAny(lowerStr(url), {
        "robots.txt", "sitemap", "favicon", ".well-known", "ads.txt",
        "humans.txt", "security.txt", "apple-touch-icon", "browserconfig.xml", "/feed"});
}

// Appartenance d'un fichier à un site : logique déplacée dans le module cœur
// LogDiscovery (portable, testable). Conservée ici comme mince adaptateur pour
// ne pas alourdir les nombreux points d'appel de l'interface.
bool fileBelongsToSite(const std::string& filename, const std::string& name,
                       const std::string& logMatch = "") {
    return logdiscovery::belongsToSite(filename, name, logMatch);
}

// Calcule les statistiques d'un site sur une période (sans conserver les entrées).
Stats analyzeSiteStats(const std::string& cacheRoot, const SiteConfig& site,
                       const std::string& fromStr, const std::string& toStr) {
    CacheManager cache(cacheRoot);
    StatsEngine engine;
    for (const std::string& file : cache.localLogs(site.name)) {
        if (!fileBelongsToSite(fs::path(file).filename().string(), site.name, site.logMatch)) continue;
        GzReader::readLines(file, [&](const std::string& line) {
            LogEntry e = ApacheLogParser::parseLine(line);
            if (e.valid && e.date >= fromStr && e.date <= toStr) engine.ingest(e);
        });
    }
    return engine.result();
}

// Remplace le dernier widget d'un layout par un nouveau.
void replaceLast(QLayout* layout, QWidget* fresh) {
    if (QLayoutItem* item = layout->takeAt(layout->count() - 1)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    if (auto* box = qobject_cast<QBoxLayout*>(layout)) {
        box->addWidget(fresh, 1);
    } else {
        layout->addWidget(fresh);
    }
}

} // namespace

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("SiteWatch");
    resize(1180, 720);
    buildUi();
    loadConfiguration();
}

QWidget* MainWindow::makeKpiCard(icons::Glyph glyph, const QString& title,
                                 const QString& subRole, Kpi& out) {
    auto* card = new QFrame;
    card->setObjectName("card");
    card->setMinimumHeight(74);
    auto* h = new QHBoxLayout(card);
    h->setContentsMargins(14, 10, 14, 10);
    h->setSpacing(12);

    // Icône de la carte : police d'icônes embarquée, teintée par le thème selon
    // la nature de l'indicateur (propriété « kpi », cf. app.qss).
    auto* icon = new QLabel(icons::ch(glyph));
    icon->setObjectName("cardIcon");
    icon->setProperty("kpi", subRole);
    h->addWidget(icon);

    auto* col = new QVBoxLayout;
    col->setSpacing(2);
    auto* titleLbl = new QLabel(title);
    titleLbl->setObjectName("cardTitle");
    col->addWidget(titleLbl);

    auto* line = new QHBoxLayout;
    line->setSpacing(8);
    out.value = new QLabel("-");
    out.value->setObjectName("cardValue");
    out.sub = new QLabel;
    out.sub->setProperty("kpi", subRole);   // teinté par le thème (voir app.qss)
    line->addWidget(out.value);
    line->addWidget(out.sub);
    line->addStretch();
    col->addLayout(line);
    h->addLayout(col);
    h->addStretch();
    return card;
}

void MainWindow::buildUi() {
    // --- Barre de menus ---
    QMenu* fichier = menuBar()->addMenu("Fichier");
    QAction* actConfig = fichier->addAction("Configuration…");
    actConfig->setShortcut(QKeySequence("Ctrl+,"));
    connect(actConfig, &QAction::triggered, this, &MainWindow::onOpenSettings);
    QAction* actClean = fichier->addAction("Effacer les logs téléchargés…");
    connect(actClean, &QAction::triggered, this, &MainWindow::onCleanCache);
    fichier->addSeparator();
    QAction* actQuit = fichier->addAction("Quitter");
    actQuit->setShortcut(QKeySequence::Quit);
    connect(actQuit, &QAction::triggered, this, &QWidget::close);

    QMenu* outils = menuBar()->addMenu("Outils");
    QAction* actSync = outils->addAction("Synchroniser maintenant");
    connect(actSync, &QAction::triggered, this, &MainWindow::onSync);

    // --- Affichage → Thème (Système / Clair / Sombre) ---
    QMenu* affichage = menuBar()->addMenu("Affichage");
    QMenu* themeMenu = affichage->addMenu("Thème");
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    struct { const char* label; Theme::Mode mode; } themes[] = {
        {"Système", Theme::Mode::System},
        {"Clair",   Theme::Mode::Light},
        {"Sombre",  Theme::Mode::Dark},
    };
    const Theme::Mode current = Theme::instance().mode();
    for (const auto& t : themes) {
        QAction* act = themeMenu->addAction(t.label);
        act->setCheckable(true);
        act->setChecked(t.mode == current);
        themeGroup->addAction(act);
        const Theme::Mode m = t.mode;
        connect(act, &QAction::triggered, this, [m] { Theme::instance().apply(m); });
    }

    QMenu* aide = menuBar()->addMenu("Aide");
    QAction* actDoc = aide->addAction("Documentation");
    actDoc->setShortcut(QKeySequence::HelpContents);   // F1
    connect(actDoc, &QAction::triggered, this, &MainWindow::onHelp);
    aide->addSeparator();
    QAction* actAbout = aide->addAction("À propos de SiteWatch");
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAbout);

    auto* central = new QWidget;
    central->setObjectName("central");
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(14, 12, 14, 8);
    mainLayout->setSpacing(12);

    // --- En-tete : site + analyser + periode ---
    auto* topBar = new QHBoxLayout;
    topBar->setSpacing(8);
    topBar->addWidget(new QLabel("Site :"));
    siteSelector_ = new QComboBox;
    siteSelector_->setMinimumWidth(180);
    topBar->addWidget(siteSelector_);
    auto* syncBtn = new QPushButton("Télécharger les logs");
    syncBtn->setToolTip("Télécharger les nouveaux logs depuis le serveur (SFTP) puis analyser");
    connect(syncBtn, &QPushButton::clicked, this, &MainWindow::onSync);
    topBar->addWidget(syncBtn);
    auto* analyzeBtn = new QPushButton("Analyser");
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyze);
    topBar->addWidget(analyzeBtn);
    topBar->addStretch();

    topBar->addWidget(new QLabel("Période :"));
    fromDate_ = new QDateEdit;
    fromDate_->setDisplayFormat("dd/MM/yyyy");
    fromDate_->setCalendarPopup(true);
    fromDate_->setMinimumWidth(120);
    fromDate_->setDate(QDate::currentDate().addDays(-60));
    topBar->addWidget(fromDate_);
    topBar->addWidget(new QLabel("→"));
    toDate_ = new QDateEdit;
    toDate_->setDisplayFormat("dd/MM/yyyy");
    toDate_->setCalendarPopup(true);
    toDate_->setMinimumWidth(120);
    toDate_->setDate(QDate::currentDate());
    topBar->addWidget(toDate_);

    presetCombo_ = new QComboBox;
    presetCombo_->addItems({"Personnalisé", "Aujourd'hui", "Hier",
                            "7 derniers jours", "30 derniers jours",
                            "Ce mois-ci", "Mois dernier",
                            "Cette année", "Année dernière"});
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPresetChanged);
    topBar->addWidget(presetCombo_);
    auto* applyBtn = new QPushButton("Appliquer");
    applyBtn->setToolTip("Recalculer l'analyse pour la période choisie");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onAnalyze);
    topBar->addWidget(applyBtn);
    mainLayout->addLayout(topBar);

    // --- Bannière intégrée (messages non bloquants) ---
    banner_ = new QFrame;
    banner_->setObjectName("banner");
    banner_->setVisible(false);
    auto* bannerRow = new QHBoxLayout(banner_);
    bannerRow->setContentsMargins(10, 8, 8, 8);
    bannerRow->setSpacing(8);
    bannerIcon_ = new QLabel;
    bannerIcon_->setObjectName("bannerIcon");
    bannerRow->addWidget(bannerIcon_, 0, Qt::AlignTop);
    bannerText_ = new QLabel;
    bannerText_->setObjectName("bannerText");
    bannerText_->setWordWrap(true);
    bannerText_->setTextInteractionFlags(Qt::TextBrowserInteraction);
    bannerText_->setOpenExternalLinks(false);
    bannerRow->addWidget(bannerText_, 1);
    bannerAction_ = new QPushButton;
    bannerAction_->setObjectName("bannerAction");
    bannerAction_->setVisible(false);
    connect(bannerAction_, &QPushButton::clicked, this, [this] {
        // Copie locale : l'action peut réinitialiser bannerActionFn_ (via
        // hideBanner/showBanner) pendant son exécution.
        if (bannerActionFn_) { auto fn = bannerActionFn_; fn(); }
    });
    bannerRow->addWidget(bannerAction_, 0, Qt::AlignVCenter);
    bannerClose_ = new QToolButton;
    bannerClose_->setObjectName("bannerClose");
    bannerClose_->setText(icons::ch(icons::Glyph::Cross));
    bannerClose_->setToolTip("Masquer ce message");
    bannerClose_->setCursor(Qt::PointingHandCursor);
    connect(bannerClose_, &QToolButton::clicked, this, &MainWindow::hideBanner);
    bannerRow->addWidget(bannerClose_, 0, Qt::AlignTop);
    mainLayout->addWidget(banner_);

    // --- Ligne permanente : site / periode / fichiers / taille ---
    metaHeader_ = new QLabel("Sélectionnez un site puis cliquez sur Analyser.");
    metaHeader_->setProperty("muted", true);
    mainLayout->addWidget(metaHeader_);

    // --- Rangee de cartes KPI ---
    auto* cards = new QHBoxLayout;
    cards->setSpacing(10);
    cards->addWidget(makeKpiCard(icons::Glyph::Chart,  "Total requêtes", "neutral", kTotal_));
    cards->addWidget(makeKpiCard(icons::Glyph::User,   "Humains",        "good",    kHumans_));
    cards->addWidget(makeKpiCard(icons::Glyph::Robot,  "Robots",         "neutral", kRobots_));
    cards->addWidget(makeKpiCard(icons::Glyph::Warn,   "Erreurs 404",    "warn404", k404_));
    cards->addWidget(makeKpiCard(icons::Glyph::Shield, "Erreurs 403",    "warn403", k403_));
    cards->addWidget(makeKpiCard(icons::Glyph::Ban,    "Erreurs 500",     "danger",  k500_));
    mainLayout->addLayout(cards);

    // --- Onglets ---
    tabs_ = new QTabWidget;

    // Onglet Sites : supervision globale multi-sites.
    sitesTab_ = new QWidget;
    auto* sitesLayout = new QVBoxLayout(sitesTab_);
    sitesLayout->setSpacing(10);
    auto* sitesIntro = new QLabel(
        "Vue globale : identifier les sites nécessitant une attention avant l'analyse détaillée.");
    sitesIntro->setProperty("muted", true);
    sitesLayout->addWidget(sitesIntro);
    sitesTable_ = new QTableWidget;
    sitesTable_->setColumnCount(13);
    sitesTable_->setHorizontalHeaderLabels({
        "Site", "État", "Humains", "Robots", "Robots IA", "Google",
        "Erreurs 404", "Erreurs 500", "Attaques", "Activité WP",
        "Dernière synchronisation", "Points d'attention", "Action recommandée"
    });
    sitesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sitesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    sitesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    sitesTable_->setAlternatingRowColors(true);
    sitesTable_->setShowGrid(false);
    sitesTable_->setWordWrap(false);
    sitesTable_->setTextElideMode(Qt::ElideRight);
    sitesTable_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    sitesTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sitesTable_->verticalHeader()->setVisible(false);
    auto* sitesHeader = sitesTable_->horizontalHeader();
    sitesHeader->setSectionResizeMode(QHeaderView::Interactive);
    sitesHeader->setMinimumSectionSize(56);
    sitesHeader->setStretchLastSection(false);
    sitesHeader->setSectionsMovable(true);
    const QList<int> siteColumnWidths = {
        150, 125, 80, 80, 85, 80, 95, 95, 85, 100, 155, 260, 230
    };
    for (int c = 0; c < siteColumnWidths.size(); ++c)
        sitesTable_->setColumnWidth(c, siteColumnWidths[c]);
    sitesTable_->setSortingEnabled(true);
    sitesTable_->setToolTip(
        "Double-cliquer un site pour entrer dans son analyse détaillée. "
        "Les colonnes peuvent être redimensionnées depuis l'en-tête.");
    connect(sitesTable_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        auto* siteItem = sitesTable_->item(row, 0);
        if (!siteItem) return;
        const QString siteName = siteItem->data(Qt::UserRole + 1).toString();
        const int idx = siteSelector_->findText(siteName);
        if (idx < 0) return;
        siteSelector_->setCurrentIndex(idx);
        tabs_->setCurrentWidget(previousDetailTab_ ? previousDetailTab_ : healthTab_);
        onAnalyze();
    });
    sitesLayout->addWidget(sitesTable_, 1);
    sitesSummary_ = new QLabel("Synchroniser ou analyser pour afficher la synthèse multi-sites.");
    sitesSummary_->setWordWrap(true);
    sitesSummary_->setProperty("muted", true);
    sitesLayout->addWidget(sitesSummary_);
    tabs_->addTab(sitesTab_, "Sites");

    // Onglet Santé (en premier) : verdict global + indicateurs.
    healthTab_ = new QWidget;
    auto* healthLayout = new QVBoxLayout(healthTab_);
    healthLayout->setSpacing(12);
    verdictLabel_ = new QLabel("Lancez une analyse pour évaluer la santé du site.");
    verdictLabel_->setObjectName("verdict");
    verdictLabel_->setAlignment(Qt::AlignCenter);
    healthLayout->addWidget(verdictLabel_);
    auto* healthListWidget = new QWidget;
    healthList_ = new QVBoxLayout(healthListWidget);
    healthList_->setSpacing(8);
    healthLayout->addWidget(healthListWidget);
    healthLayout->addStretch();
    tabs_->addTab(healthTab_, "Santé");

    // Onglet Robots : arbre a gauche, donut + top robots a droite.
    robotsTab_ = new QWidget;
    auto* robotsLayout = new QHBoxLayout(robotsTab_);
    robotsLayout->setSpacing(12);

    robotsTree_ = new QTreeWidget;
    robotsTree_->setColumnCount(3);
    robotsTree_->setHeaderLabels({"Catégorie / Robot", "Requêtes", "%"});
    robotsTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    robotsTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    robotsTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    robotsTree_->setRootIsDecorated(true);
    robotsLayout->addWidget(robotsTree_, 2);

    auto* rightCol = new QVBoxLayout;
    rightCol->setSpacing(12);

    // Panneau donut.
    donutPanel_ = new QFrame;
    donutPanel_->setObjectName("panel");
    auto* donutLayout = new QVBoxLayout(donutPanel_);
    auto* donutTitle = new QLabel("Répartition par catégorie");
    donutTitle->setProperty("panelTitle", true);
    donutLayout->addWidget(donutTitle);
    donutLayout->addStretch();            // remplace par le donut a l'analyse
    rightCol->addWidget(donutPanel_, 3);

    // Panneau top robots.
    auto* topPanel = new QFrame;
    topPanel->setObjectName("panel");
    auto* topLayout = new QVBoxLayout(topPanel);
    auto* topTitle = new QLabel("Top robots (toutes catégories)");
    topTitle->setProperty("panelTitle", true);
    topLayout->addWidget(topTitle);
    topRobotsTable_ = new QTableWidget;
    topRobotsTable_->setColumnCount(3);
    topRobotsTable_->horizontalHeader()->setVisible(false);
    topRobotsTable_->verticalHeader()->setVisible(false);
    topRobotsTable_->setShowGrid(false);
    topRobotsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    topRobotsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    topRobotsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    topRobotsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    topLayout->addWidget(topRobotsTable_);
    rightCol->addWidget(topPanel, 2);

    robotsLayout->addLayout(rightCol, 3);
    tabs_->addTab(robotsTab_, "Robots");

    securityTable_ = makeTable("Tentative d'attaque / Erreur", "Occurrences");
    activityTable_ = makeTable3("Activité WordPress légitime", "Requêtes", "%");
    pagesTable_    = makeTable("Page", "Vues");
    referersTable_ = makeTable("Provenance", "Requêtes");
    attackedTable_ = makeTable("URL attaquée / 404", "Occurrences");

    tabs_->addTab(securityTable_, "Sécurité");
    tabs_->addTab(activityTable_, "Activité WP");
    tabs_->addTab(pagesTable_,    "Top pages");
    tabs_->addTab(referersTable_, "Référents");

    // Onglet URLs : catégorie d'affichage + tableau.
    urlsTab_ = new QWidget;
    auto* attackedLayout = new QVBoxLayout(urlsTab_);
    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Affichage :"));
    urlFilter_ = new QComboBox;
    urlFilter_->addItems({"Toutes les URL", "Attaques probables",
                          "Fonctionnement WordPress", "Erreurs 404", "Requêtes système"});
    urlFilter_->setCurrentIndex(1);   // "Attaques probables" par défaut
    connect(urlFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] { fillUrls(); });
    filterRow->addWidget(urlFilter_);
    filterRow->addStretch();
    attackedLayout->addLayout(filterRow);
    attackedLayout->addWidget(attackedTable_);
    tabs_->addTab(urlsTab_, "URLs");

    // Chaque onglet tabulaire est interactif : double-clic = fenêtre de détail,
    // clic droit = copier / exporter la ou les ligne(s) sélectionnée(s).
    wireDetailTable(pagesTable_,    DetailSource::Url);      // Top pages
    wireDetailTable(attackedTable_, DetailSource::Url);      // URLs
    wireDetailTable(referersTable_, DetailSource::Referer);  // Référents
    wireDetailTable(securityTable_, DetailSource::Attack);   // Sécurité
    wireDetailTable(activityTable_, DetailSource::Normal);   // Activité WP

    // Onglet graphiques.
    chartTab_ = new QWidget;
    auto* chartLayout = new QVBoxLayout(chartTab_);
    chartSelector_ = new QComboBox;
    chartSelector_->addItems({
        "Évolution du trafic (humains / robots)",
        "Évolution des robots IA",
        "Évolution des robots SEO",
        "Évolution des erreurs 404",
        "Évolution de l'activité WordPress",
        "Évolution des tentatives d'attaque",
    });
    connect(chartSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onChartChanged);
    chartLayout->addWidget(chartSelector_);
    chartLayout->addStretch();
    tabs_->addTab(chartTab_, "Graphiques");

    // Onglet Recherche : barre de recherche + résultats.
    auto* searchTab = new QWidget;
    auto* searchLayout = new QVBoxLayout(searchTab);
    auto* searchBar = new QHBoxLayout;
    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText("IP, URL, robot, date (2026-07-06) ou code HTTP…");
    auto* searchBtn = new QPushButton("Rechercher");
    connect(searchEdit_, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    searchBar->addWidget(searchEdit_);
    searchBar->addWidget(searchBtn);
    searchLayout->addLayout(searchBar);
    searchInfo_ = new QLabel;
    searchInfo_->setProperty("muted", true);
    searchLayout->addWidget(searchInfo_);
    searchTable_ = new QTableWidget;
    searchTable_->setColumnCount(7);
    searchTable_->setHorizontalHeaderLabels(
        {"Date", "Heure", "IP", "Code", "Méthode", "URL", "User-Agent"});
    searchTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    searchTable_->verticalHeader()->setVisible(false);
    searchTable_->setShowGrid(false);
    searchTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    searchTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    searchTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    searchTable_->setContextMenuPolicy(Qt::CustomContextMenu);
    searchTable_->setToolTip("Double-cliquez une ligne pour le détail de l'URL • "
                             "clic droit : copier / exporter la sélection");
    // Double-clic sur un résultat -> détail de l'URL de la ligne (colonne 5).
    connect(searchTable_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* it = searchTable_->item(row, 5)) showUrlDetail(it->text());
    });
    // Clic droit : copier / exporter les lignes brutes sélectionnées.
    connect(searchTable_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        std::vector<int> rows;
        for (const auto& idx : searchTable_->selectionModel()->selectedRows())
            rows.push_back(idx.row());
        if (rows.empty()) return;
        std::sort(rows.begin(), rows.end());
        QMenu menu(this);
        QAction* copyAct = menu.addAction(QString("Copier (%1 ligne(s))").arg(rows.size()));
        QAction* csvAct  = menu.addAction("Exporter en CSV…");
        QAction* chosen  = menu.exec(searchTable_->viewport()->mapToGlobal(p));
        if (!chosen) return;

        const int cols = searchTable_->columnCount();
        auto cell = [&](int r, int c) {
            return searchTable_->item(r, c) ? searchTable_->item(r, c)->text() : QString();
        };
        if (chosen == copyAct) {
            QStringList head;
            for (int c = 0; c < cols; ++c)
                head << searchTable_->horizontalHeaderItem(c)->text();
            QString text = head.join('\t') + "\n";
            for (int r : rows) {
                QStringList cells;
                for (int c = 0; c < cols; ++c) cells << cell(r, c);
                text += cells.join('\t') + "\n";
            }
            QApplication::clipboard()->setText(text);
        } else {
            auto field = [](QString v) {
                if (v.contains(',') || v.contains('"') || v.contains('\n')) {
                    v.replace("\"", "\"\""); v = "\"" + v + "\"";
                }
                return v;
            };
            QStringList head;   // en-têtes CSV internationaux (sans accents)
            for (int c = 0; c < cols; ++c)
                head << urlreport::deaccent(searchTable_->horizontalHeaderItem(c)->text());
            QString csv = head.join(',') + "\n";
            for (int r : rows) {
                QStringList cells;
                for (int c = 0; c < cols; ++c) cells << field(cell(r, c));
                csv += cells.join(',') + "\n";
            }
            urlreport::saveCsv(this, "recherche.csv", csv);
        }
    });
    tabs_->addTab(searchTab, "Recherche");
    previousDetailTab_ = healthTab_;
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget* current = tabs_->widget(index);
        if (current && current != sitesTab_) previousDetailTab_ = current;
    });

    mainLayout->addWidget(tabs_);
    setCentralWidget(central);

    // --- Barre de statut ---
    statusBar()->showMessage("Prêt.");
    progressBar_ = new QProgressBar;
    progressBar_->setMaximumWidth(220);
    progressBar_->setTextVisible(true);
    progressBar_->setVisible(false);
    statusBar()->addPermanentWidget(progressBar_);
    statusRight_ = new QLabel;
    statusBar()->addPermanentWidget(statusRight_);

    // Les graphiques (QChart) ne sont pas couverts par le QSS : on les
    // reconstruit au changement de thème pour ré-accorder textes et séparateurs.
    connect(&Theme::instance(), &Theme::changed, this, [this] {
        if (lastStats_.totalRequests > 0) {
            rebuildDonut(lastStats_);
            rebuildChart();
        }
    });
}

const SiteConfig* MainWindow::currentSite() const {
    const QString name = siteSelector_->currentText();
    for (const auto& s : config_.sites)
        if (QString::fromStdString(s.name) == name) return &s;
    return nullptr;
}

SiteConfig* MainWindow::currentSiteMutable() {
    const QString name = siteSelector_->currentText();
    for (auto& s : config_.sites)
        if (QString::fromStdString(s.name) == name) return &s;
    return nullptr;
}

// Force la ré-application de la feuille de style après un changement de
// propriété dynamique (level="…"), sinon Qt ne recalcule pas les couleurs.
static void repolish(QWidget* w) {
    if (!w) return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void MainWindow::showBanner(BannerLevel level, const QString& html,
                            const QString& actionText,
                            std::function<void()> onAction) {
    const char* levelName = "info";
    icons::Glyph glyph = icons::Glyph::CircleInfo;
    switch (level) {
        case BannerLevel::Success: levelName = "success"; glyph = icons::Glyph::CircleCheck;  break;
        case BannerLevel::Warning: levelName = "warn";    glyph = icons::Glyph::Warn;         break;
        case BannerLevel::Error:   levelName = "error";   glyph = icons::Glyph::CrossCircle;  break;
        case BannerLevel::Info:    levelName = "info";    glyph = icons::Glyph::CircleInfo;   break;
    }
    banner_->setProperty("level", levelName);
    bannerText_->setProperty("level", levelName);
    bannerIcon_->setProperty("level", levelName);
    bannerIcon_->setText(icons::ch(glyph));
    bannerText_->setText(html);

    bannerActionFn_ = std::move(onAction);
    const bool hasAction = !actionText.isEmpty() && bannerActionFn_;
    bannerAction_->setVisible(hasAction);
    if (hasAction) bannerAction_->setText(actionText);

    repolish(banner_);
    repolish(bannerText_);
    repolish(bannerIcon_);
    banner_->setVisible(true);
}

void MainWindow::hideBanner() {
    banner_->setVisible(false);
    bannerActionFn_ = nullptr;
}

void MainWindow::applySuggestedFilter(const QString& filter) {
    SiteConfig* site = currentSiteMutable();
    if (!site) return;
    site->logMatch = filter.toStdString();

    std::string saveErr;
    if (!Config::save(configFilePath().toStdString(), config_, saveErr)) {
        showBanner(BannerLevel::Error,
            "Impossible d'enregistrer le filtre : " +
            QString::fromStdString(saveErr).toHtmlEscaped());
        return;
    }
    // Le filtre est enregistré : on relance la synchronisation, qui va cette
    // fois retenir les bons fichiers puis les télécharger.
    onSync();
}

QString MainWindow::configFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/config.json";
}

void MainWindow::populateSiteSelector() {
    const QString previous = siteSelector_->currentText();
    siteSelector_->clear();
    for (const auto& site : config_.sites)
        siteSelector_->addItem(QString::fromStdString(site.name));
    const int idx = siteSelector_->findText(previous);
    if (idx >= 0) siteSelector_->setCurrentIndex(idx);
}

void MainWindow::loadConfiguration() {
    const QString path = configFilePath();

    // Migration : si rien à l'emplacement standard, on récupère un ancien
    // config.json (dossier de l'exe, dossier parent, ou dossier courant).
    if (!QFileInfo::exists(path)) {
        const QStringList legacy = {
            QApplication::applicationDirPath() + "/config.json",
            QApplication::applicationDirPath() + "/../config.json",
            QStringLiteral("config.json")
        };
        for (const QString& old : legacy)
            if (QFileInfo::exists(old)) { QFile::copy(old, path); break; }
    }

    std::string err;
    if (!Config::load(path.toStdString(), config_, err)) {
        configError_ = QString::fromStdString(err);
        statusBar()->showMessage(
            "Aucune configuration — menu Fichier → Configuration… pour la créer.");
        return;
    }
    configError_.clear();
    populateSiteSelector();
    refreshSitesOverview();
    if (config_.sites.empty())
        statusBar()->showMessage("Aucun site défini — Fichier → Configuration…");
}

void MainWindow::onOpenSettings() {
    SettingsDialog dlg(config_, this);
    if (dlg.exec() != QDialog::Accepted) return;

    config_ = dlg.result();
    std::string err;
    if (!Config::save(configFilePath().toStdString(), config_, err)) {
        QMessageBox::warning(this, "Configuration", QString::fromStdString(err));
        return;
    }
    configError_.clear();
    populateSiteSelector();
    refreshSitesOverview();
    statusBar()->showMessage("Configuration enregistrée : " + configFilePath());
}

void MainWindow::onPresetChanged() {
    const QDate today = QDate::currentDate();
    QDate from = fromDate_->date(), to = today;
    switch (presetCombo_->currentIndex()) {
        case 0: return;                                            // Personnalisé
        case 1: from = today; break;                               // Aujourd'hui
        case 2: from = today.addDays(-1); to = today.addDays(-1); break; // Hier
        case 3: from = today.addDays(-7); break;                   // 7 jours
        case 4: from = today.addDays(-30); break;                  // 30 jours
        case 5: from = QDate(today.year(), today.month(), 1); break; // Ce mois
        case 6: {                                                  // Mois dernier
            QDate firstThis(today.year(), today.month(), 1);
            to = firstThis.addDays(-1);
            from = QDate(to.year(), to.month(), 1);
            break;
        }
        case 7: from = QDate(today.year(), 1, 1); break;           // Cette annee
        case 8: from = QDate(today.year() - 1, 1, 1);              // Annee derniere
                to = QDate(today.year() - 1, 12, 31); break;
    }
    fromDate_->setDate(from);
    toDate_->setDate(to);
}

bool MainWindow::whitelistSsh(const SiteConfig& site, QString& error) {
    // Sans jeton d'API, on ne fait rien : l'utilisateur autorise son IP à la main.
    if (site.cpanelToken.empty()) return true;

    QNetworkAccessManager nam;

    // Petit utilitaire : requête GET synchrone (via boucle d'événements locale).
    auto syncGet = [&nam](const QNetworkRequest& req, QByteArray& body) -> QNetworkReply::NetworkError {
        QNetworkReply* r = nam.get(req);
        QEventLoop loop;
        connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        body = r->readAll();
        auto err = r->error();
        r->deleteLater();
        return err;
    };

    // 1. IP publique courante.
    QByteArray ipBody;
    if (syncGet(QNetworkRequest(QUrl("https://api.ipify.org")), ipBody) != QNetworkReply::NoError) {
        error = "Impossible de déterminer l'IP publique (api.ipify.org).";
        return false;
    }
    const QString ip = QString::fromUtf8(ipBody).trimmed();

    // 2. Ajout de l'IP à la liste blanche SSH (port 22) via l'API cPanel.
    QUrl url(QString("https://%1:2083/execute/SshWhitelist/add")
                 .arg(QString::fromStdString(site.host)));
    QUrlQuery q;
    q.addQueryItem("address", ip);
    q.addQueryItem("port", "22");
    url.setQuery(q);

    QNetworkRequest req(url);
    const QByteArray auth = "cpanel " +
        QByteArray::fromStdString(site.user) + ":" + QByteArray::fromStdString(site.cpanelToken);
    req.setRawHeader("Authorization", auth);

    QByteArray body;
    if (syncGet(req, body) != QNetworkReply::NoError) {
        error = "Autorisation pare-feu refusée (jeton d'API invalide ?).";
        return false;
    }
    // cPanel renvoie du JSON : status=1 en cas de succès.
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject() && doc.object().value("status").toInt() != 1) {
        error = "Réponse cPanel : " + QString::fromUtf8(body).left(200);
        return false;
    }

    // Laisse le temps à la règle de se propager (o2switch : jusqu'à ~35 s).
    QEventLoop wait;
    QTimer::singleShot(4000, &wait, &QEventLoop::quit);
    wait.exec();
    return true;
}

void MainWindow::onSync() {
    hideBanner();   // repartir d'un état propre à chaque tentative

    if (!configError_.isEmpty() || config_.sites.empty()) {
        showBanner(BannerLevel::Error,
            "Aucune configuration valide. Ouvrez <b>Fichier → Configuration…</b> "
            "pour renseigner au moins un site.");
        return;
    }

    const SiteConfig* site = currentSite();
    if (!site) return;
    const QString siteLabel = QString::fromStdString(site->name).toHtmlEscaped();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Étape préalable : autoriser l'IP courante sur le pare-feu o2switch.
    if (!site->cpanelToken.empty()) {
        statusBar()->showMessage("Autorisation du pare-feu o2switch (API cPanel) …");
        QApplication::processEvents();
        QString wlErr;
        if (!whitelistSsh(*site, wlErr)) {
            QApplication::restoreOverrideCursor();
            statusBar()->showMessage("Échec de l'autorisation pare-feu.");
            showBanner(BannerLevel::Error,
                "<b>Pare-feu o2switch : autorisation refusée.</b><br>"
                "Vérifiez le <b>jeton d'API cPanel</b> dans la configuration du site. "
                "Détail : " + wlErr.toHtmlEscaped());
            return;
        }
    }

    statusBar()->showMessage("Connexion SFTP à " + QString::fromStdString(site->host) + " …");
    QApplication::processEvents();

    SftpClient client;
    std::string err;
    if (!client.connect(*site, err)) {
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage("Échec de la connexion SFTP.");
        showBanner(BannerLevel::Error,
            "<b>Connexion SFTP impossible</b> à " +
            QString::fromStdString(site->host).toHtmlEscaped() + ".<br>"
            "Vérifiez l'hôte, l'utilisateur et le mot de passe (ou la clé SSH). "
            "Détail : " + QString::fromStdString(err).toHtmlEscaped());
        return;
    }

    auto remote = client.listLogs(site->remoteLogDir, err);
    if (remote.empty() && !err.empty()) {
        QApplication::restoreOverrideCursor();
        client.disconnect();
        statusBar()->showMessage("Dossier distant illisible.");
        showBanner(BannerLevel::Error,
            "<b>Dossier distant illisible :</b> " +
            QString::fromStdString(site->remoteLogDir).toHtmlEscaped() + ".<br>"
            "Vérifiez le chemin « Dossier distant des logs » dans la configuration. "
            "Détail : " + QString::fromStdString(err).toHtmlEscaped());
        return;
    }

    // Diagnostic : le dossier contient-il des logs, et correspondent-ils au
    // site (filtre/domaine) ? On distingue « rien » de « présents mais filtrés ».
    std::vector<std::string> remoteNames;
    remoteNames.reserve(remote.size());
    for (const auto& rf : remote) remoteNames.push_back(rf.name);
    const logdiscovery::Report diag = logdiscovery::analyze(remoteNames, *site);

    if (diag.outcome == logdiscovery::Outcome::NoFilesInDir) {
        QApplication::restoreOverrideCursor();
        client.disconnect();
        statusBar()->showMessage("Aucun log sur le serveur.");
        showBanner(BannerLevel::Warning,
            "<b>Aucun log (.gz) dans le dossier distant</b> " +
            QString::fromStdString(site->remoteLogDir).toHtmlEscaped() + ".<br>"
            "Vérifiez le dossier des logs, ou attendez la rotation des logs de "
            "votre hébergeur (les fichiers compressés apparaissent souvent le lendemain).");
        return;
    }

    if (diag.outcome == logdiscovery::Outcome::NoneMatch) {
        QApplication::restoreOverrideCursor();
        client.disconnect();
        statusBar()->showMessage("Aucun fichier ne correspond au filtre.");
        const QString current = site->logMatch.empty()
            ? QStringLiteral("détection automatique")
            : QStringLiteral("« %1 »").arg(
                  QString::fromStdString(site->logMatch).toHtmlEscaped());
        QString msg =
            QString("<b>%1 fichier(s) présent(s) sur le serveur, mais aucun ne "
                    "correspond au filtre actuel (%2).</b>")
                .arg(diag.totalGz).arg(current);
        if (diag.suggestion.valid) {
            const QString filter =
                QString::fromStdString(diag.suggestion.filter);
            msg += QString("<br>SiteWatch a détecté un préfixe commun : <b>%1</b> "
                           "(%2 fichier(s)).")
                       .arg(filter.toHtmlEscaped()).arg(diag.suggestion.wouldMatch);
            showBanner(BannerLevel::Warning, msg,
                       "Utiliser ce filtre : " + filter,
                       [this, filter] { applySuggestedFilter(filter); });
        } else {
            msg += "<br>Renseignez le champ <b>Filtre des logs (avancé)</b> dans "
                   "la configuration du site pour cibler les bons fichiers.";
            showBanner(BannerLevel::Warning, msg);
        }
        return;
    }

    // Outcome::Matched : des fichiers correspondent → téléchargement.
    // 1er passage : ne retenir que les logs de CE site (filtre domaine) et
    //               non déjà présents en cache avec la même taille.
    CacheManager cache(config_.cacheRoot);
    const std::string dir = cache.siteDir(site->name);
    std::vector<SftpClient::RemoteFile> toDownload;
    int skipped = 0, ignored = 0;
    unsigned long long grandTotal = 0;
    for (const auto& rf : remote) {
        if (!fileBelongsToSite(rf.name, site->name, site->logMatch)) {
            ++ignored;                       // autre domaine ou sous-domaine
            continue;
        }
        std::error_code ec;
        const std::string local = dir + "/" + rf.name;
        if (fs::exists(local, ec) && fs::file_size(local, ec) == rf.size) {
            ++skipped;                       // déjà en cache, inchangé : pas de re-téléchargement
            continue;
        }
        toDownload.push_back(rf);
        grandTotal += rf.size;
    }

    // 2e passage : téléchargement avec barre de progression globale (en octets).
    int downloaded = 0, failed = 0;
    if (!toDownload.empty()) {
        progressBar_->setRange(0, 100);
        progressBar_->setValue(0);
        progressBar_->setVisible(true);
        unsigned long long doneBase = 0;
        for (const auto& rf : toDownload) {
            statusBar()->showMessage(
                QString("Téléchargement de %1 …").arg(QString::fromStdString(rf.name)));
            QApplication::processEvents();
            const std::string local = dir + "/" + rf.name;
            const std::string remotePath = site->remoteLogDir + "/" + rf.name;
            const bool ok = client.download(
                remotePath, local, err, rf.size,
                [&](uint64_t received, uint64_t) {
                    const unsigned long long overall = doneBase + received;
                    const int p = grandTotal
                        ? static_cast<int>(overall * 100ULL / grandTotal) : 100;
                    progressBar_->setValue(p);
                    QApplication::processEvents();
                });
            doneBase += rf.size;
            if (ok) ++downloaded; else ++failed;
        }
        progressBar_->setValue(100);
        progressBar_->setVisible(false);
    }

    client.disconnect();
    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(
        QString("SFTP : %1 téléchargé(s), %2 en cache, %3 autre(s) domaine(s) ignoré(s)%4.")
            .arg(downloaded).arg(skipped).arg(ignored)
            .arg(failed ? QString(", %1 échec(s)").arg(failed) : QString()));

    // Bannière de synthèse (non bloquante).
    if (failed > 0 && downloaded == 0) {
        showBanner(BannerLevel::Error,
            QString("<b>Échec du téléchargement</b> pour %1. "
                    "Réessayez ou vérifiez la connexion.").arg(siteLabel));
    } else if (downloaded > 0) {
        QString msg = QString("<b>%1 : %2 fichier(s) de logs téléchargé(s).</b>")
                          .arg(siteLabel).arg(downloaded);
        if (skipped) msg += QString(" %1 déjà à jour.").arg(skipped);
        if (failed)  msg += QString(" %1 échec(s).").arg(failed);
        showBanner(BannerLevel::Success, msg);
    } else {
        showBanner(BannerLevel::Success,
            QString("<b>%1 : déjà à jour.</b> Aucun nouveau fichier à télécharger "
                    "(%2 en cache).").arg(siteLabel).arg(skipped));
    }

    onAnalyze();   // analyse immédiate des logs mis à jour
    refreshSitesOverview();
    tabs_->setCurrentWidget(sitesTab_);
}

void MainWindow::onAnalyze() {
    if (!configError_.isEmpty() || config_.sites.empty()) {
        QMessageBox::warning(this, "SiteWatch",
            "Aucune configuration valide.\n\nCopiez config.example.json en "
            "config.json et renseignez vos sites.");
        return;
    }

    const std::string siteName = siteSelector_->currentText().toStdString();
    const std::string fromStr = fromDate_->date().toString("yyyy-MM-dd").toStdString();
    const std::string toStr   = toDate_->date().toString("yyyy-MM-dd").toStdString();

    CacheManager cache(config_.cacheRoot);
    std::vector<std::string> logs = cache.localLogs(siteName);

    // Ne garder que les fichiers de ce site (même filtre que le téléchargement).
    if (const SiteConfig* site = currentSite(); site && !site->name.empty()) {
        logs.erase(std::remove_if(logs.begin(), logs.end(),
            [&](const std::string& path) {
                return !fileBelongsToSite(fs::path(path).filename().string(),
                                          site->name, site->logMatch);
            }), logs.end());
    }

    if (logs.empty()) {
        statusBar()->showMessage(
            QString("Aucun fichier .gz dans %1/%2 (téléchargement SFTP en Phase 2).")
                .arg(QString::fromStdString(config_.cacheRoot),
                     QString::fromStdString(siteName)));
        return;
    }

    // --- Pipeline coeur : lecture -> parsing -> filtre de date -> stats ---
    StatsEngine engine;
    entries_.clear();
    unsigned long long totalBytes = 0;
    for (const std::string& file : logs) {
        std::error_code ec;
        totalBytes += fs::file_size(file, ec);
        GzReader::readLines(file, [&](const std::string& line) {
            LogEntry e = ApacheLogParser::parseLine(line);
            if (e.valid && e.date >= fromStr && e.date <= toStr) {
                engine.ingest(e);
                entries_.push_back(e);   // conservé pour le détail au double-clic
            }
        });
    }

    lastStats_ = engine.result();
    displayStats(lastStats_);

    // Ligne d'en-tete permanente.
    metaHeader_->setText(QString(
        "Site : %1      Période : %2 → %3      %4 fichier(s) - %5")
        .arg(QString::fromStdString(siteName),
             fromDate_->date().toString("dd/MM/yyyy"),
             toDate_->date().toString("dd/MM/yyyy"))
        .arg(logs.size())
        .arg(humanSize(totalBytes)));

    statusBar()->showMessage("Analyse terminée.");
    statusRight_->setText("Dernière analyse : " +
        QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") + "  ✓");
    refreshSitesOverview();
}

void MainWindow::displayStats(const Stats& s) {
    const long t = s.totalRequests;
    kTotal_.value->setText(num(t));
    kHumans_.value->setText(num(s.humans));
    kHumans_.sub->setText(pct(s.humans, t));
    kRobots_.value->setText(num(s.bots));
    kRobots_.sub->setText(pct(s.bots, t));
    k404_.value->setText(num(s.errors404));
    k404_.sub->setText(pct(s.errors404, t));
    k403_.value->setText(num(s.errors403));
    k403_.sub->setText(pct(s.errors403, t));
    k500_.value->setText(num(s.errors500));
    k500_.sub->setText(pct(s.errors500, t));

    fillHealth(s);
    fillRobotsTree(s);
    fillTopRobots(s);
    rebuildDonut(s);

    {
        auto rows = sortedByValue(s.attackActivity);
        rows.emplace_back("Erreurs 404", s.errors404);
        rows.emplace_back("Erreurs 403", s.errors403);
        rows.emplace_back("Erreurs 500", s.errors500);
        // Conserve le libellé brut (sans pastille) pour retrouver les entrées
        // de la catégorie au double-clic / à l'export.
        std::vector<QString> raw;
        raw.reserve(rows.size());
        for (auto& r : rows) raw.push_back(r.first);
        setRows(securityTable_, rows);
        for (int i = 0; i < static_cast<int>(raw.size()); ++i)
            if (auto* it = securityTable_->item(i, 0)) {
                it->setData(Qt::UserRole, raw[i]);
                it->setIcon(icons::icon(icons::Glyph::Dot, stateColor(isSevereLabel(raw[i]) ? 2 : 1)));
            }
    }
    setRows3(activityTable_, sortedByValue(s.normalActivity), t);
    setRows(pagesTable_,    sortedByValue(s.topPages, 50));
    setRows(referersTable_, sortedByValue(s.referers));
    fillUrls();

    rebuildChart();
}

void MainWindow::refreshSitesOverview() {
    if (!sitesTable_ || !sitesSummary_) return;
    sitesTable_->setSortingEnabled(false);
    sitesTable_->setRowCount(0);

    if (configError_.size() > 0 || config_.sites.empty()) {
        sitesSummary_->setText("Aucun site configuré.");
        return;
    }

    const std::string fromStr = fromDate_->date().toString("yyyy-MM-dd").toStdString();
    const std::string toStr = toDate_->date().toString("yyyy-MM-dd").toStdString();
    CacheManager cache(config_.cacheRoot);

    struct Row {
        QString name;
        Stats stats;
        int level = 0;
        QDateTime lastSync;
    };
    std::vector<Row> rows;
    rows.reserve(config_.sites.size());

    for (const auto& site : config_.sites) {
        Row row;
        row.name = QString::fromStdString(site.name);
        row.stats = analyzeSiteStats(config_.cacheRoot, site, fromStr, toStr);
        row.level = healthLevel(row.stats);

        for (const std::string& file : cache.localLogs(site.name)) {
            if (!fileBelongsToSite(fs::path(file).filename().string(), site.name, site.logMatch))
                continue;
            const QDateTime modified = QFileInfo(QString::fromStdString(file)).lastModified();
            if (modified.isValid() && (!row.lastSync.isValid() || modified > row.lastSync))
                row.lastSync = modified;
        }
        rows.push_back(row);
    }

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.level != b.level) return a.level > b.level;
        return a.name.toLower() < b.name.toLower();
    });

    sitesTable_->setRowCount(static_cast<int>(rows.size()));
    int r = 0;
    for (const Row& row : rows) {
        const Stats& s = row.stats;
        const long attacks = sumAttacks(s);
        const long ai = aiBots(s);
        const long google = botOf(s, "Google");
        const long wp = normalActivityTotal(s);

        auto* name = textItem(row.name, row.name.toLower());
        name->setData(Qt::UserRole + 1, row.name);
        sitesTable_->setItem(r, 0, name);
        auto* stateItem = textItem(healthLabel(row.level), row.level);
        stateItem->setIcon(icons::icon(icons::Glyph::Dot, stateColor(row.level)));
        sitesTable_->setItem(r, 1, stateItem);
        sitesTable_->setItem(r, 2, numberItem(s.humans));
        sitesTable_->setItem(r, 3, numberItem(s.bots));
        sitesTable_->setItem(r, 4, numberItem(ai));
        sitesTable_->setItem(r, 5, numberItem(google));
        sitesTable_->setItem(r, 6, numberItem(s.errors404));
        sitesTable_->setItem(r, 7, numberItem(s.errors500));
        sitesTable_->setItem(r, 8, numberItem(attacks));
        sitesTable_->setItem(r, 9, numberItem(wp));
        sitesTable_->setItem(r, 10, textItem(
            row.lastSync.isValid() ? row.lastSync.toString("dd/MM/yyyy HH:mm") : "Jamais",
            row.lastSync.isValid() ? row.lastSync.toSecsSinceEpoch() : 0));
        sitesTable_->setItem(r, 11, textItem(attentionSummary(s)));
        sitesTable_->setItem(r, 12, textItem(recommendedAction(s)));
        ++r;
    }
    sitesTable_->setSortingEnabled(true);

    auto maxBy = [&](auto getter) -> QString {
        const Row* best = nullptr;
        long bestValue = -1;
        for (const Row& row : rows) {
            const long value = getter(row.stats);
            if (value > bestValue) { bestValue = value; best = &row; }
        }
        return best && bestValue > 0 ? QString("%1 (%2)").arg(best->name, num(bestValue)) : "Aucun";
    };
    const auto bestHealth = std::min_element(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.level != b.level) return a.level < b.level;
        return a.name.toLower() < b.name.toLower();
    });

    Theme& th = Theme::instance();
    const int ip = 13;
    auto item = [&](icons::Glyph g, const QString& colorTok, const QString& label,
                    const QString& value) {
        return icons::span(g, th.color(colorTok), ip) + " " + label + " : " + value;
    };
    sitesSummary_->setText(QStringList{
        item(icons::Glyph::Trophy,  "accent",    "Site le plus visité",
             maxBy([](const Stats& s) { return s.humans; })),
        item(icons::Glyph::Shield,  "danger",    "Site le plus attaqué",
             maxBy([](const Stats& s) { return sumAttacks(s); })),
        item(icons::Glyph::Robot,   "textMuted", "Robots IA",
             maxBy([](const Stats& s) { return aiBots(s); })),
        item(icons::Glyph::CrossCircle, "kpi404", "Erreurs 404",
             maxBy([](const Stats& s) { return s.errors404; })),
        item(icons::Glyph::TrendUp, "accent",    "Activité Google",
             maxBy([](const Stats& s) { return botOf(s, "Google"); })),
        item(icons::Glyph::Heart,   "ok",        "Meilleur état",
             bestHealth != rows.end() ? bestHealth->name : QString("Aucun")),
    }.join("   ·   "));
}

void MainWindow::fillUrls() {
    const int mode = urlFilter_ ? urlFilter_->currentIndex() : 0;
    std::map<std::string, long> agg;
    for (const auto& e : entries_) {
        bool keep = false;
        switch (mode) {
            case 0: keep = true;                        break;  // Toutes les URL
            case 1: keep = isProbableAttack(e.url);     break;  // Attaques probables
            case 2: keep = isWordPressInternal(e.url);  break;  // Fonctionnement WordPress
            case 3: keep = (e.status == 404);           break;  // Erreurs 404
            case 4: keep = isSystemRequest(e.url);      break;  // Requêtes système
        }
        if (keep) ++agg[e.url];
    }
    std::vector<std::pair<QString, long>> rows;
    rows.reserve(agg.size());
    for (const auto& [url, n] : agg)
        rows.emplace_back(QString::fromStdString(url), n);
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (rows.size() > 100) rows.resize(100);
    setRows(attackedTable_, rows);
}

void MainWindow::showUrlDetail(const QString& url) {
    openDetail(DetailSource::Url, url);
}

std::vector<const LogEntry*> MainWindow::matchesFor(DetailSource src, const QString& label) const {
    std::vector<const LogEntry*> out;
    const std::string lab = label.toStdString();
    // Onglet Sécurité : les lignes "Erreurs 404/403/500" filtrent par code HTTP.
    const bool byError = (src == DetailSource::Attack) && label.startsWith("Erreurs ");
    const int errCode = byError ? label.mid(QStringLiteral("Erreurs ").size()).toInt() : 0;

    for (const auto& e : entries_) {
        bool keep = false;
        switch (src) {
            case DetailSource::Url:
                keep = (e.url == lab);
                break;
            case DetailSource::Referer:
                // Les référents ne sont comptés que pour les humains (cf. StatsEngine).
                keep = !BotDetector::detect(e.userAgent).isBot
                       && StatsEngine::classifyReferer(e.referer) == lab;
                break;
            case DetailSource::Attack:
                if (byError) {
                    keep = (e.status == errCode);
                } else {
                    const auto a = StatsEngine::classifyActivity(e);
                    keep = (a.type == StatsEngine::ActivityType::Attack && a.label == lab);
                }
                break;
            case DetailSource::Normal: {
                const auto a = StatsEngine::classifyActivity(e);
                keep = (a.type == StatsEngine::ActivityType::Normal && a.label == lab);
                break;
            }
        }
        if (keep) out.push_back(&e);
    }
    return out;
}

std::vector<urlreport::Group> MainWindow::groupsFor(QTableWidget* table, DetailSource src,
                                                    const std::vector<int>& rows) const {
    std::vector<urlreport::Group> groups;
    groups.reserve(rows.size());
    for (int r : rows) {
        auto* it = table->item(r, 0);
        if (!it) continue;
        const QString label = it->data(Qt::UserRole).isValid()
                                  ? it->data(Qt::UserRole).toString() : it->text();
        groups.push_back({label, matchesFor(src, label)});
    }
    return groups;
}

void MainWindow::openDetail(DetailSource src, const QString& label) {
    if (label.isEmpty() || entries_.empty()) return;
    QString prefix;
    switch (src) {
        case DetailSource::Url:     prefix = "URL";       break;
        case DetailSource::Referer: prefix = "Référent";  break;
        case DetailSource::Attack:  prefix = "Catégorie"; break;
        case DetailSource::Normal:  prefix = "Activité";  break;
    }
    DetailDialog dlg(prefix + " : " + label, siteSelector_->currentText(),
                     matchesFor(src, label), this);
    dlg.exec();
}

void MainWindow::wireDetailTable(QTableWidget* table, DetailSource src) {
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    table->setToolTip("Double-cliquez une ligne pour le détail • "
                      "clic droit : copier / exporter la sélection");

    connect(table, &QTableWidget::cellDoubleClicked, this, [this, table, src](int row, int) {
        if (auto* it = table->item(row, 0)) {
            const QString label = it->data(Qt::UserRole).isValid()
                                      ? it->data(Qt::UserRole).toString() : it->text();
            openDetail(src, label);
        }
    });

    connect(table, &QWidget::customContextMenuRequested, this, [this, table, src](const QPoint& p) {
        std::vector<int> rows;
        for (const auto& idx : table->selectionModel()->selectedRows())
            rows.push_back(idx.row());
        if (rows.empty()) return;
        std::sort(rows.begin(), rows.end());
        const auto groups = groupsFor(table, src, rows);
        QMenu menu(this);
        QAction* copyAct = menu.addAction(QString("Copier les infos (%1 ligne(s))").arg(rows.size()));
        QAction* csvAct  = menu.addAction("Exporter en CSV…");
        QAction* chosen  = menu.exec(table->viewport()->mapToGlobal(p));
        if (chosen == copyAct)
            QApplication::clipboard()->setText(urlreport::groupsText(groups));
        else if (chosen == csvAct)
            urlreport::saveCsv(this, "export.csv", urlreport::groupsCsv(groups));
    });
}

void MainWindow::onSearch() {
    searchTable_->setRowCount(0);
    const QString q = searchEdit_->text().trimmed();
    if (q.isEmpty() || entries_.empty()) { searchInfo_->setText(""); return; }

    const std::string needle = q.toLower().toStdString();
    bool numeric = false;
    const int code = q.toInt(&numeric);

    std::vector<const LogEntry*> matches;
    for (const auto& e : entries_) {
        const bool m = (numeric && e.status == code)
            || lowerStr(e.ip).find(needle)        != std::string::npos
            || lowerStr(e.url).find(needle)       != std::string::npos
            || lowerStr(e.userAgent).find(needle) != std::string::npos
            || e.date.find(needle)                != std::string::npos;
        if (m) {
            matches.push_back(&e);
            if (matches.size() >= 2000) break;
        }
    }

    searchTable_->setRowCount(static_cast<int>(matches.size()));
    for (int i = 0; i < static_cast<int>(matches.size()); ++i) {
        const LogEntry& e = *matches[i];
        searchTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(e.date)));
        searchTable_->setItem(i, 1, new QTableWidgetItem(
            e.hour >= 0 ? QString::asprintf("%02dh", e.hour) : QString("-")));
        searchTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(e.ip)));
        searchTable_->setItem(i, 3, new QTableWidgetItem(QString::number(e.status)));
        searchTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(e.method)));
        searchTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(e.url)));
        searchTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(e.userAgent)));
    }
    searchInfo_->setText(QString("%1 résultat(s)%2")
        .arg(matches.size())
        .arg(matches.size() >= 2000 ? " (limité à 2000)" : ""));
}

void MainWindow::onCleanCache() {
    QStringList names;
    for (const auto& s : config_.sites) names << QString::fromStdString(s.name);
    CacheCleanupDialog dlg(QString::fromStdString(config_.cacheRoot), names, this);
    dlg.exec();
}

void MainWindow::onHelp() {
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle("Aide — SiteWatch");
    box.setTextFormat(Qt::RichText);
    box.setText(
        "<b>Prise en main</b>"
        "<ol>"
        "<li><b>Fichier → Configuration…</b> : ajoutez vos sites (serveur SFTP, "
        "utilisateur, clé SSH, jeton d'API cPanel, nom du site).</li>"
        "<li><b>Synchroniser</b> télécharge les nouveaux logs puis analyse ; "
        "<b>Analyser</b> traite les logs déjà en cache.</li>"
        "<li>Choisissez la <b>période</b> en haut à droite — tous les onglets se recalculent.</li>"
        "<li>Onglets : <b>Sites</b>, Santé, Robots, Sécurité, Activité WP, Top pages, "
        "Référents, <b>URLs</b> (filtre par catégorie), Graphiques, <b>Recherche</b>.</li>"
        "<li><b>Sites</b> donne la vue globale : priorité, points d'attention et action recommandée.</li>"
        "<li><b>Double-cliquez</b> n'importe quelle ligne (Sécurité, Activité WP, "
        "Top pages, Référents, URLs, Recherche) pour son détail (IP, User-Agents, "
        "URLs, horaires…) ; <b>clic droit</b> pour copier / exporter la sélection "
        "(presse-papier ou CSV).</li>"
        "</ol>");
    box.exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "À propos de SiteWatch",
        QString(
            "<b>SiteWatch %1</b><br>"
            "Analyseur de logs Apache/LiteSpeed pour l'administration et la "
            "supervision de sites web.<br><br>"
            "Analyse locale des <code>.gz</code>, téléchargement SFTP, tableau de "
            "santé, détection de robots et d'attaques.<br><br>"
            "Conçu pour l'hébergement <b>o2switch</b> (pare-feu SSH cPanel, "
            "découpage mensuel des logs) — adaptable à d'autres hébergeurs.<br><br>"
            "© 2026 <b>morfredus</b><br>"
            "<span style=\"color:#888;\">Licensed under the GNU GPL v3</span>")
            .arg(SITEWATCH_VERSION));
}

void MainWindow::fillHealth(const Stats& s) {
    const long total = s.totalRequests;

    const long attacks = sumAttacks(s);
    const long google = botOf(s, "Google");
    const long ai = aiBots(s);
    auto ratio = [&](long n) { return total > 0 ? 100.0 * n / total : 0.0; };

    // Un indicateur : état (0 vert, 1 orange, 2 rouge), titre, valeur, remarque.
    struct Ind { int st; QString title; QString val; QString note; };
    std::vector<Ind> inds;

    { int st = (s.errors500 == 0) ? 0 : (s.errors500 <= 10) ? 1 : 2;
      inds.push_back({st, "Erreurs 500", num(s.errors500),
          st == 0 ? "Aucune erreur serveur" : "Erreurs serveur à surveiller"}); }

    { double p = ratio(attacks); int st = (p < 1.0) ? 0 : (p < 5.0) ? 1 : 2;
      inds.push_back({st, "Tentatives d'attaque",
          QString("%1 (%2)").arg(num(attacks), pct(attacks, total)),
          st == 0 ? "Niveau de sondage normal" : "Activité malveillante élevée"}); }

    { double p = ratio(s.errors404); int st = (p < 2.0) ? 0 : (p < 10.0) ? 1 : 2;
      inds.push_back({st, "Erreurs 404",
          QString("%1 (%2)").arg(num(s.errors404), pct(s.errors404, total)),
          st == 0 ? "Peu de liens cassés" : "Beaucoup de pages introuvables"}); }

    { int st = 0;
      inds.push_back({st, "Activité Google", num(google) + " requêtes",
          google > 0 ? "Googlebot explore le site" : "Googlebot n'a pas visité le site"}); }

    inds.push_back({0, "Robots IA", num(ai) + " requêtes",
                    "Claude, OpenAI, Perplexity (informatif)"});

    // Verdict global = même calcul que l'onglet Sites.
    const int worst = healthLevel(s);

    QString text, state;
    auto withDot = [](int lvl, const QString& words) {
        return icons::span(icons::Glyph::Dot, stateHex(lvl), 18) + "  " + words;
    };
    if (total == 0) {
        text = "Aucune donnée à analyser sur cette période.";
        state = "neutral";
    } else if (worst == 2) {
        text = withDot(2, "Attention — anomalies détectées");
        state = "danger";
    } else if (worst == 1) {
        text = withDot(1, "Quelques anomalies");
        state = "warn";
    } else {
        text = withDot(0, "Aucune anomalie détectée");
        state = "ok";
    }
    verdictLabel_->setText(text);
    // Change l'état puis force la ré-application du QSS (propriété dynamique).
    verdictLabel_->setProperty("state", state);
    verdictLabel_->style()->unpolish(verdictLabel_);
    verdictLabel_->style()->polish(verdictLabel_);

    // Reconstruit la liste des indicateurs.
    while (QLayoutItem* item = healthList_->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    for (size_t i = 0; i < inds.size(); ++i) {
        const auto& ind = inds[i];
        auto* row = new QFrame;
        row->setObjectName("panel");
        row->setProperty("healthIndex", static_cast<int>(i));
        row->setCursor(Qt::PointingHandCursor);
        row->setToolTip("Double-cliquez pour ouvrir l'onglet correspondant");
        row->installEventFilter(this);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(12, 8, 12, 8);
        auto* dotLbl = new QLabel(icons::ch(icons::Glyph::Dot));
        dotLbl->setFont(icons::font(14));
        dotLbl->setStyleSheet("color:" + stateHex(ind.st) + ";");
        h->addWidget(dotLbl);
        auto* titleLbl = new QLabel("<b>" + ind.title + "</b>");
        titleLbl->setMinimumWidth(180);
        h->addWidget(titleLbl);
        auto* valLbl = new QLabel(ind.val);
        valLbl->setMinimumWidth(140);
        h->addWidget(valLbl);
        auto* noteLbl = new QLabel(ind.note);
        noteLbl->setProperty("muted", true);
        h->addWidget(noteLbl);
        h->addStretch();
        // Les libellés laissent passer les clics vers le cadre (pour le double-clic).
        for (QLabel* l : {dotLbl, titleLbl, valLbl, noteLbl})
            l->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        healthList_->addWidget(row);
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick) {
        const QVariant idx = obj->property("healthIndex");
        if (idx.isValid()) {
            navigateHealth(idx.toInt());
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::navigateHealth(int index) {
    switch (index) {
        case 0: tabs_->setCurrentWidget(securityTable_); break;        // Erreurs 500
        case 1: tabs_->setCurrentWidget(urlsTab_);                     // Tentatives d'attaque
                if (urlFilter_) urlFilter_->setCurrentIndex(1);
                break;
        case 2: tabs_->setCurrentWidget(urlsTab_);                     // Erreurs 404
                if (urlFilter_) urlFilter_->setCurrentIndex(3);
                break;
        case 3: tabs_->setCurrentWidget(robotsTab_); break;            // Activité Google
        case 4: tabs_->setCurrentWidget(robotsTab_); break;            // Robots IA
        default: break;
    }
}

void MainWindow::fillRobotsTree(const Stats& s) {
    robotsTree_->clear();
    const long total = s.totalRequests;
    for (const auto& cat : kRobotCategories) {
        long subtotal = 0;
        std::vector<std::pair<QString, long>> children;
        for (const QString& engine : cat.engines) {
            auto it = s.botCounts.find(engine.toStdString());
            long n = (it != s.botCounts.end()) ? it->second : 0;
            if (n > 0) { children.emplace_back(engine, n); subtotal += n; }
        }
        if (subtotal == 0) continue;

        auto* parent = new QTreeWidgetItem(robotsTree_);
        parent->setText(0, cat.name);
        parent->setText(1, num(subtotal));
        parent->setText(2, pct(subtotal, total));
        parent->setTextAlignment(1, Qt::AlignRight);
        parent->setTextAlignment(2, Qt::AlignRight);
        QFont f = parent->font(0); f.setBold(true);
        for (int c = 0; c < 3; ++c) parent->setFont(c, f);

        std::sort(children.begin(), children.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        for (const auto& [name, n] : children) {
            auto* child = new QTreeWidgetItem(parent);
            child->setText(0, name);
            child->setText(1, num(n));
            child->setText(2, pct(n, total));
            child->setTextAlignment(1, Qt::AlignRight);
            child->setTextAlignment(2, Qt::AlignRight);
        }
        parent->setExpanded(true);
    }
}

void MainWindow::fillTopRobots(const Stats& s) {
    auto rows = sortedByValue(s.botCounts, 8);
    topRobotsTable_->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        topRobotsTable_->setItem(i, 0,
            new QTableWidgetItem(QString("%1. %2").arg(i + 1).arg(rows[i].first)));
        auto* c = new QTableWidgetItem(num(rows[i].second));
        c->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        topRobotsTable_->setItem(i, 1, c);
        auto* p = new QTableWidgetItem("(" + pct(rows[i].second, s.totalRequests) + ")");
        p->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        p->setForeground(QColor(Theme::instance().color("textMuted")));
        topRobotsTable_->setItem(i, 2, p);
    }
}

void MainWindow::rebuildDonut(const Stats& s) {
    auto* series = new QPieSeries;
    series->setHoleSize(0.42);   // trou plus petit -> anneau plus épais
    series->setPieSize(0.82);    // diamètre plus grand -> donut plus lisible
    std::vector<QColor> sliceColors;
    for (const auto& cat : kRobotCategories) {
        long subtotal = 0;
        for (const QString& engine : cat.engines) {
            auto it = s.botCounts.find(engine.toStdString());
            if (it != s.botCounts.end()) subtotal += it->second;
        }
        if (subtotal == 0) continue;
        series->append(QString("%1 (%2)").arg(cat.name, pct(subtotal, s.totalRequests)),
                       static_cast<double>(subtotal));
        sliceColors.emplace_back(cat.color);
    }

    auto* chart = new QChart;
    chart->setTheme(Theme::instance().isDark() ? QChart::ChartThemeDark
                                               : QChart::ChartThemeLight);
    chart->addSeries(series);

    // Après addSeries : le thème du graphique a pu réattribuer les couleurs.
    // On (ré)impose les couleurs de catégorie + la séparation ton surface.
    const QColor sep(Theme::instance().color("surface"));
    const auto slices = series->slices();
    for (int i = 0; i < slices.size() && i < static_cast<int>(sliceColors.size()); ++i) {
        slices[i]->setColor(sliceColors[i]);
        slices[i]->setLabelVisible(false);
        slices[i]->setPen(QPen(sep, 2));
    }
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));

    auto* view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(200);
    replaceLast(donutPanel_->layout(), view);
}

void MainWindow::onChartChanged() { rebuildChart(); }

void MainWindow::rebuildChart() {
    QString title;
    std::vector<std::pair<QString, const std::map<std::string, long>*>> defs;
    switch (chartSelector_->currentIndex()) {
        case 0: title = "Évolution du trafic";
                defs = {{"Humains", &lastStats_.dailyHumans},
                        {"Robots",  &lastStats_.dailyBots}}; break;
        case 1: title = "Robots IA";  defs = {{"Robots IA",  &lastStats_.dailyAI}}; break;
        case 2: title = "Robots SEO"; defs = {{"Robots SEO", &lastStats_.dailySEO}}; break;
        case 3: title = "Erreurs 404"; defs = {{"Erreurs 404", &lastStats_.daily404}}; break;
        case 4: title = "Activité WordPress"; defs = {{"Activité WP", &lastStats_.dailyNormal}}; break;
        case 5: title = "Tentatives d'attaque"; defs = {{"Attaques", &lastStats_.dailyAttacks}}; break;
    }

    std::set<std::string> dateSet;
    for (const auto& [name, m] : defs)
        for (const auto& [d, c] : *m) dateSet.insert(d);
    std::vector<std::string> dates(dateSet.begin(), dateSet.end());

    if (dates.empty()) {
        auto* empty = new QLabel("Aucune donnée à afficher pour ce graphique sur la période sélectionnée.");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(320);
        empty->setProperty("muted", true);
        replaceLast(chartTab_->layout(), empty);
        return;
    }

    auto* chart = new QChart;
    chart->setTheme(Theme::instance().isDark() ? QChart::ChartThemeDark
                                               : QChart::ChartThemeLight);
    chart->setTitle(title);
    chart->setBackgroundVisible(false);
    double maxY = 0.0;
    for (const auto& [name, m] : defs) {
        auto* serie = new QLineSeries;
        serie->setName(name);
        serie->setPointsVisible(true);
        for (size_t i = 0; i < dates.size(); ++i) {
            auto it = m->find(dates[i]);
            const double y = it != m->end() ? static_cast<double>(it->second) : 0.0;
            maxY = std::max(maxY, y);
            serie->append(static_cast<double>(i), y);
        }
        chart->addSeries(serie);
    }
    chart->createDefaultAxes();
    if (auto axes = chart->axes(Qt::Horizontal); !axes.empty()) {
        if (auto* axis = qobject_cast<QValueAxis*>(axes.first())) {
            axis->setRange(dates.size() == 1 ? -0.5 : 0.0,
                           dates.size() == 1 ? 0.5 : static_cast<double>(dates.size() - 1));
            axis->setTickCount(static_cast<int>(std::min<size_t>(dates.size(), 8)) + 1);
        }
    }
    if (auto axes = chart->axes(Qt::Vertical); !axes.empty()) {
        if (auto* axis = qobject_cast<QValueAxis*>(axes.first())) {
            axis->setRange(0.0, std::max(1.0, maxY * 1.15));
        }
    }
    chart->legend()->setVisible(true);

    auto* view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(320);
    replaceLast(chartTab_->layout(), view);
}
