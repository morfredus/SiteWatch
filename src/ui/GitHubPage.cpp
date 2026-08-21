/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/GitHubPage.h"
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

QByteArray waitReply(QNetworkReply* reply, int timeoutMs) {
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    if (reply->isRunning())
        reply->abort();
    return reply->readAll();
}

QString analyticsBase(QString url) {
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    return url;
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

void GitHubPage::reconcileFromCollector() {
    if (!config_.github.enabled || collectorUrl_.isEmpty())
        return;
    if (!ensureStore())
        return;
    catchUpFromCollector();
    refresh();
}

void GitHubPage::catchUpFromCollector() {
    if (collectorUrl_.isEmpty() || !store_ || !store_->isOpen())
        return;
    for (const GitHubRepoConfig& repo : config_.github.repositories) {
        if (!repo.enabled)
            continue;
        const QString full = QString::fromStdString(config_.github.owner + "/" + repo.name);
        const QString sid = QStringLiteral("sitewatch:github:") + full;
        const CollectorClient::Reply objs = CollectorClient::getObjects(collectorUrl_, sid);
        for (const QJsonValue& v : objs.json.value(QStringLiteral("objects")).toArray()) {
            const QString oid = v.toObject().value(QStringLiteral("object_id")).toString();
            if (oid.isEmpty())
                continue;
            bool ok = false;
            QString err;
            const QByteArray raw = CollectorClient::download(collectorUrl_, oid, ok, err);
            if (!ok)
                continue;
            const QJsonObject snap = QJsonDocument::fromJson(raw).object();
            if (snap.value(QStringLiteral("contract")).toString() != QLatin1String("github-traffic/1"))
                continue;
            store_->ingestSnapshot(snap, QStringLiteral("morfcollector"));
        }
    }
}

void GitHubPage::publishToAnalytics() {
    if (!ensureStore() || !store_->hasConsolidatedData())
        return;
    publishAuthority();
}

void GitHubPage::publishAuthority() {
    if (!ensureStore())
        return;
    if (analyticsUrl_.isEmpty()) {
        next_->setText(QStringLiteral(
            "Données locales prêtes. morfAnalytics n'est pas encore vu sur le réseau : "
            "la page Analyses GitHub restera vide jusqu'à la publication."));
        return;
    }
    const QString base = analyticsBase(analyticsUrl_);
    QNetworkAccessManager nam;

    QNetworkRequest stReq(QUrl(base + QStringLiteral("/status")));
    stReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply* stReply = nam.get(stReq);
    const QJsonObject status = QJsonDocument::fromJson(waitReply(stReply, 8000)).object();
    const int stHttp = stReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    stReply->deleteLater();
    const QString app = status.value(QStringLiteral("app")).toString();
    if (stHttp != 200 || app.isEmpty()) {
        next_->setText(QStringLiteral(
            "Publication : %1 ne répond pas comme morfAnalytics (HTTP %2). "
            "Vérifiez le port 8799, pas le beacon 8787.")
                           .arg(base).arg(stHttp));
        return;
    }

    const QByteArray payload =
        QJsonDocument(store_->exportAuthority()).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(base + QStringLiteral("/github/ingest")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply* reply = nam.post(req, payload);
    const QByteArray raw = waitReply(reply, 20000);
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonObject body = QJsonDocument::fromJson(raw).object();
    QString msg;
    if (http == 200 && body.value(QStringLiteral("ok")).toBool())
        msg = QStringLiteral("Vérité GitHub publiée vers %1 (v%2).")
                  .arg(base, status.value(QStringLiteral("version")).toString());
    else if (http == 405)
        msg = QStringLiteral(
            "Publication refusée (HTTP 405) sur %1 : ce morfAnalytics est trop ancien "
            "ou n'expose pas POST /github/ingest. Mettez à jour le service.")
                  .arg(base);
    else {
        const QString detail = body.value(QStringLiteral("detail")).toString();
        const QString err = body.value(QStringLiteral("error")).toString();
        msg = QStringLiteral("Publication refusée (HTTP %1) sur %2%3")
                  .arg(http)
                  .arg(base)
                  .arg(err.isEmpty() && detail.isEmpty()
                           ? QString()
                           : QStringLiteral(" : ") + (detail.isEmpty() ? err : detail));
    }
    reply->deleteLater();
    next_->setText(msg);
}
