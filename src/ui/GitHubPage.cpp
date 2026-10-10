/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/GitHubPage.h"
#include "github/GitHubSync.h"
#include "collector/CollectorClient.h"
#include "github/GitHubAuthorityStore.h"
#include "github/GitHubTrafficClient.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>

namespace {

class GitHubSortItem : public QTableWidgetItem {
public:
    explicit GitHubSortItem(const QString& text) : QTableWidgetItem(text) {}
    bool operator<(const QTableWidgetItem& other) const override {
        const QVariant left = data(Qt::UserRole);
        const QVariant right = other.data(Qt::UserRole);
        if (left.isValid() && right.isValid()) {
            bool okL = false, okR = false;
            const double l = left.toDouble(&okL);
            const double r = right.toDouble(&okR);
            if (okL && okR)
                return l < r;
            return left.toString() < right.toString();
        }
        return text() < other.text();
    }
};

GitHubSortItem* textCell(const QString& text, const QVariant& sort = {}) {
    auto* item = new GitHubSortItem(text);
    if (sort.isValid())
        item->setData(Qt::UserRole, sort);
    return item;
}

GitHubSortItem* numCell(qint64 value) {
    auto* item = new GitHubSortItem(QString::number(value));
    item->setData(Qt::UserRole, static_cast<qlonglong>(value));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

} // namespace

GitHubPage::GitHubPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    auto* title = new QLabel(QStringLiteral("GitHub - métriques des dépôts suivis"));
    title->setObjectName(QStringLiteral("pageTitle"));
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral(
        "SiteWatch interroge GitHub et consolide. Actualiser relit le connu. "
        "Collecter maintenant va chercher les mesures, puis met l'écran à jour. "
        "morfCollector archive le brut et comble les absences. "
        "morfAnalytics n'analyse que cette vérité consolidée."));
    hint->setProperty("muted", true);
    hint->setWordWrap(true);
    root->addWidget(hint);

    status_ = new QLabel(QStringLiteral("Aucune collecte GitHub configurée."));
    status_->setWordWrap(true);
    root->addWidget(status_);

    auto* peerRow = new QHBoxLayout;
    peerRow->addWidget(new QLabel(QStringLiteral("Collecteur :")));
    collectorPick_ = new QComboBox;
    collectorPick_->setMinimumWidth(240);
    connect(collectorPick_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (fillingPeers_)
            return;
        emit collectorChosen(collectorPick_->currentData().toString());
    });
    peerRow->addWidget(collectorPick_, 1);
    peerRow->addWidget(new QLabel(QStringLiteral("Analyses :")));
    analyticsPick_ = new QComboBox;
    analyticsPick_->setMinimumWidth(240);
    connect(analyticsPick_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (fillingPeers_)
            return;
        emit analyticsChosen(analyticsPick_->currentData().toString());
    });
    peerRow->addWidget(analyticsPick_, 1);
    root->addLayout(peerRow);
    collectorLabel_ = new QLabel;
    collectorLabel_->setProperty("muted", true);
    collectorLabel_->setWordWrap(true);
    root->addWidget(collectorLabel_);
    analyticsLabel_ = new QLabel;
    analyticsLabel_->setProperty("muted", true);
    analyticsLabel_->setWordWrap(true);
    root->addWidget(analyticsLabel_);
    next_ = new QLabel;
    next_->setProperty("muted", true);
    next_->setWordWrap(true);
    root->addWidget(next_);

    auto* row = new QHBoxLayout;
    collectBtn_ = new QPushButton(QStringLiteral("Collecter maintenant"));
    collectBtn_->setToolTip(QStringLiteral(
        "Interroge GitHub, enregistre, complète depuis morfCollector si besoin, puis actualise."));
    connect(collectBtn_, &QPushButton::clicked, this, &GitHubPage::collectNow);
    row->addWidget(collectBtn_);
    catchUpBtn_ = new QPushButton(QStringLiteral("Rapatrier depuis le collecteur"));
    catchUpBtn_->setToolTip(QStringLiteral(
        "Récupère les instantanés déjà collectés par morfCollector qui manquent en local, "
        "sans appeler GitHub (utile même sans jeton), puis publie la vérité vers morfAnalytics."));
    connect(catchUpBtn_, &QPushButton::clicked, this, &GitHubPage::pullFromCollector);
    row->addWidget(catchUpBtn_);
    auto* refreshBtn = new QPushButton(QStringLiteral("Actualiser"));
    refreshBtn->setToolTip(QStringLiteral("Recharge les données déjà consolidées, sans appeler GitHub."));
    connect(refreshBtn, &QPushButton::clicked, this, &GitHubPage::refresh);
    row->addWidget(refreshBtn);
    pushBtn_ = new QPushButton(QStringLiteral("Envoyer la config"));
    pushBtn_->setToolTip(QStringLiteral(
        "Pousse sites + GitHub vers le collecteur choisi (comme l'onglet Sites)."));
    connect(pushBtn_, &QPushButton::clicked, this, &GitHubPage::pushConfigRequested);
    row->addWidget(pushBtn_);
    analyticsBtn_ = new QPushButton(QStringLiteral("Analyses avancées GitHub"));
    analyticsBtn_->setEnabled(false);
    connect(analyticsBtn_, &QPushButton::clicked, this, [this] {
        publishAuthority();
        emit openAnalyticsRequested();
    });
    row->addWidget(analyticsBtn_);
    row->addStretch();
    root->addLayout(row);

    table_ = new QTableWidget(0, 8);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("Dépôt"), QStringLiteral("État"),
        QStringLiteral("Dernière collecte"), QStringLiteral("Vues 14 j"),
        QStringLiteral("Visiteurs (fenêtre GitHub)"), QStringLiteral("Clones"),
        QStringLiteral("Téléchargements (cumul)"), QStringLiteral("Dernière release")
    });
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionsClickable(true);
    table_->setSortingEnabled(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setToolTip(QStringLiteral("Cliquer sur un en-tête pour trier la colonne."));
    root->addWidget(table_, 1);
}

GitHubPage::~GitHubPage() = default;

void GitHubPage::setConfig(const Config& cfg) {
    const QString path = QDir::cleanPath(QString::fromStdString(cfg.cacheRoot)
        + QStringLiteral("/github/github.sqlite"));
    if (store_ && storePath_ != path)
        store_.reset();
    config_ = cfg;
    storePath_ = path;
    rebuildPeerCombos();
}

void GitHubPage::setCollectorUrl(const QString& url) {
    collectorUrl_ = url;
    updatePeerLabels();
    rebuildPeerCombos();
}

void GitHubPage::setAnalyticsUrl(const QString& url) {
    analyticsUrl_ = url;
    analyticsBtn_->setEnabled(!url.isEmpty());
    analyticsBtn_->setToolTip(url.isEmpty()
        ? QStringLiteral("morfAnalytics n'est pas disponible")
        : QStringLiteral("Ouvrir %1/github").arg(url));
    updatePeerLabels();
    rebuildPeerCombos();
}

void GitHubPage::setDiscoveredPeers(const QMap<QString, QString>& collectors,
                                    const QMap<QString, QString>& analytics) {
    collectors_ = collectors;
    analytics_ = analytics;
    rebuildPeerCombos();
    updatePeerLabels();
}

void GitHubPage::setNotice(const QString& text) {
    next_->setText(text);
}

void GitHubPage::rebuildPeerCombos() {
    if (!collectorPick_ || !analyticsPick_)
        return;
    fillingPeers_ = true;
    collectorPick_->clear();
    collectorPick_->addItem(QStringLiteral("Automatique (1er détecté)"), QString());
    for (auto it = collectors_.constBegin(); it != collectors_.constEnd(); ++it) {
        const QString app = it.value().isEmpty() ? QStringLiteral("morfCollector") : it.value();
        collectorPick_->addItem(QStringLiteral("%1 - %2").arg(app, it.key()), it.key());
    }
    const QString pinnedCol = QString::fromStdString(config_.collectorUrl);
    int cidx = collectorPick_->findData(pinnedCol);
    collectorPick_->setCurrentIndex(cidx >= 0 ? cidx : 0);

    analyticsPick_->clear();
    analyticsPick_->addItem(QStringLiteral("Automatique (même hôte que le collecteur)"), QString());
    for (auto it = analytics_.constBegin(); it != analytics_.constEnd(); ++it) {
        const QString app = it.value().isEmpty() ? QStringLiteral("morfAnalytics") : it.value();
        analyticsPick_->addItem(QStringLiteral("%1 - %2").arg(app, it.key()), it.key());
    }
    const QString pinnedAn = QString::fromStdString(config_.analyticsUrl);
    int aidx = analyticsPick_->findData(pinnedAn);
    analyticsPick_->setCurrentIndex(aidx >= 0 ? aidx : 0);
    fillingPeers_ = false;
}

void GitHubPage::updatePeerLabels() {
    auto describe = [](const QMap<QString, QString>& seen, const QString& url) {
        if (url.isEmpty())
            return QStringLiteral("aucun");
        const QString name = seen.value(url);
        return name.isEmpty() ? url : QStringLiteral("%1 (%2)").arg(name, url);
    };
    if (collectorLabel_)
        collectorLabel_->setText(QStringLiteral("Collecteur lu : %1")
                                     .arg(describe(collectors_, collectorUrl_)));
    if (analyticsLabel_)
        analyticsLabel_->setText(QStringLiteral("Analyses avancées : %1")
                                     .arg(describe(analytics_, analyticsUrl_)));
}

bool GitHubPage::ensureStore() {
    if (config_.cacheRoot.empty()) {
        next_->setText(QStringLiteral(
            "Racine de cache absente : impossible d'ouvrir le magasin GitHub."));
        return false;
    }
    const QString path = QDir::cleanPath(QString::fromStdString(config_.cacheRoot)
        + QStringLiteral("/github/github.sqlite"));
    if (store_ && storePath_ != path)
        store_.reset();
    storePath_ = path;
    if (!store_)
        store_ = std::make_unique<GitHubAuthorityStore>(path);
    if (store_->isOpen())
        return true;
    if (!store_->open()) {
        next_->setText(QStringLiteral("Stockage GitHub indisponible : ") + store_->lastError());
        return false;
    }
    return true;
}

void GitHubPage::refresh() {
    table_->setRowCount(0);
    if (!config_.github.enabled || config_.github.owner.empty()) {
        status_->setText(QStringLiteral(
            "Collecte GitHub désactivée. Activez-la dans Configuration → GitHub."));
        next_->clear();
        return;
    }

    int followed = 0;
    for (const GitHubRepoConfig& repo : config_.github.repositories) {
        if (repo.enabled)
            ++followed;
    }
    status_->setText(QStringLiteral("Propriétaire : %1 - %2 dépôt(s) suivi(s).")
                         .arg(QString::fromStdString(config_.github.owner))
                         .arg(followed));
    next_->setText(QStringLiteral(
        "Actualiser relit le connu. Collecter maintenant interroge GitHub."));
    if (followed == 0) {
        next_->setText(QStringLiteral(
            "Aucun dépôt coché. Ouvrez Configuration → GitHub, cochez, Envoyer la config."));
        return;
    }
    if (!ensureStore())
        return;

    table_->setSortingEnabled(false);
    int row = 0;
    for (const GitHubRepoConfig& repo : config_.github.repositories) {
        if (!repo.enabled)
            continue;
        const QString full = QString::fromStdString(config_.github.owner + "/" + repo.name);
        const GitHubAuthorityStore::Row st = store_ && store_->isOpen()
            ? store_->row(full) : GitHubAuthorityStore::Row{};
        table_->insertRow(row);
        table_->setItem(row, 0, textCell(full));
        QString state = st.lastCollectTs > 0
            ? (st.provenance + QStringLiteral(" / consolidé"))
            : QStringLiteral("pas encore collecté");
        if (!st.lastError.isEmpty())
            state = st.lastError;
        QString lastCollect = QStringLiteral("-");
        if (st.lastCollectTs > 0)
            lastCollect = QDateTime::fromSecsSinceEpoch(st.lastCollectTs)
                              .toString(QStringLiteral("dd/MM/yyyy HH:mm"));
        table_->setItem(row, 1, textCell(state));
        table_->setItem(row, 2, textCell(lastCollect, static_cast<qlonglong>(st.lastCollectTs)));
        table_->setItem(row, 3, numCell(st.views14));
        table_->setItem(row, 4, numCell(st.uniques14));
        table_->setItem(row, 5, numCell(st.clones));
        table_->setItem(row, 6, numCell(st.downloads));
        table_->setItem(row, 7, textCell(
            st.lastRelease.isEmpty() ? QStringLiteral("-") : st.lastRelease));
        ++row;
    }
    table_->setSortingEnabled(true);
    table_->resizeColumnsToContents();
}

void GitHubPage::collectNow() {
    if (!config_.github.enabled || config_.github.owner.empty()) {
        next_->setText(QStringLiteral("Activez GitHub dans Configuration avant de collecter."));
        return;
    }
    const QString token = QString::fromStdString(config_.github.token);
    if (token.isEmpty()) {
        next_->setText(QStringLiteral("Jeton GitHub manquant dans la configuration."));
        return;
    }
    if (!ensureStore())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    int ok = 0, fail = 0;
    QString lastErr;
    const QString owner = QString::fromStdString(config_.github.owner);
    for (const GitHubRepoConfig& repo : config_.github.repositories) {
        if (!repo.enabled)
            continue;
        const GitHubTrafficClient::Result got =
            GitHubTrafficClient::fetch(owner, QString::fromStdString(repo.name), token);
        if (!got.ok) {
            ++fail;
            lastErr = got.error;
            continue;
        }
        store_->ingestSnapshot(got.snapshot, QStringLiteral("github-direct"));
        ++ok;
        QApplication::processEvents();
    }
    catchUpFromCollector();
    publishAuthority();
    QApplication::restoreOverrideCursor();
    next_->setText(QStringLiteral("Collecte directe : %1 ok, %2 échec%3")
                       .arg(ok).arg(fail)
                       .arg(lastErr.isEmpty() ? QString() : QStringLiteral(" (") + lastErr + QLatin1Char(')')));
    refresh();
}

int GitHubPage::applyCatchUp(const QVector<QJsonObject>& snapshots) {
    if (!config_.github.enabled)
        return 0;
    if (!ensureStore())
        return 0;
    const int newDays = githubsync::ingest(*store_, snapshots);
    refresh();
    if (newDays > 0)
        publishAuthority();   // les donnees locales ont avance : morfAnalytics doit suivre
    return newDays;
}

int GitHubPage::reconcileFromCollector() {
    if (!config_.github.enabled || collectorUrl_.isEmpty())
        return 0;
    if (!ensureStore())
        return 0;
    const int newDays = catchUpFromCollector();
    refresh();
    // Les données locales ont avancé : morfAnalytics (s'il est là) doit suivre.
    if (newDays > 0)
        publishAuthority();
    return newDays;
}

// Bouton dedie : rapatrie ce que morfCollector a deja archive (sur le Pi ou tout
// autre hote ou tourne le collecteur), sans jamais appeler l'API GitHub. Utile si
// le jeton n'est pas configure ici, ou simplement pour materialiser dans SiteWatch
// les collectes quotidiennes accumulees pendant qu'on n'ouvrait pas l'appli.
void GitHubPage::pullFromCollector() {
    if (!config_.github.enabled || config_.github.owner.empty()) {
        next_->setText(QStringLiteral(
            "Activez GitHub dans Configuration avant de rapatrier."));
        return;
    }
    if (collectorUrl_.isEmpty()) {
        next_->setText(QStringLiteral(
            "Aucun morfCollector détecté sur le réseau : rien à rapatrier. "
            "Vérifiez que le collecteur est en ligne, puis Actualiser."));
        return;
    }
    if (!ensureStore())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const int got = catchUpFromCollector();
    const QString pub = publishAuthority();
    QApplication::restoreOverrideCursor();
    refresh();

    const QString head = got > 0
        ? QStringLiteral("Rapatriement : %1 jour(s) nouveau(x) lus depuis le collecteur, "
                         "vérité consolidée mise à jour.").arg(got)
        : QStringLiteral("Rapatriement : rien de nouveau sur le collecteur "
                         "(morfCollector collecte une fois par jour).");
    next_->setText(pub.isEmpty() ? head : head + QLatin1Char(' ') + pub);
}

int GitHubPage::catchUpFromCollector() {
    if (!store_ || !store_->isOpen())
        return 0;
    return githubsync::catchUp(config_, collectorUrl_, *store_);
}

void GitHubPage::publishToAnalytics() {
    if (!ensureStore() || !store_->hasConsolidatedData())
        return;
    publishAuthority();
}

QString GitHubPage::publishAuthority() {
    if (!ensureStore())
        return QString();
    if (analyticsUrl_.isEmpty()) {
        const QString msg = QStringLiteral(
            "Données locales prêtes. morfAnalytics n'est pas encore vu sur le réseau : "
            "la page Analyses GitHub restera vide jusqu'à la publication.");
        next_->setText(msg);
        return msg;
    }
    bool ok = false;
    const QString msg = githubsync::publish(analyticsUrl_, *store_, ok);
    next_->setText(msg);
    return msg;
}
