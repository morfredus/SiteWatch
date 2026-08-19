/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/CoherenceDialog.h"

#include "coherence/CoherenceEngine.h"
#include "collector/CollectorClient.h"
#include "core/net/SftpClient.h"
#include "core/cache/CacheManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QColor>
#include <QFile>

#include <fstream>

using coherence::FileRow;
using coherence::FileState;
using coherence::SiteReport;

namespace {

// Taille lisible : "—" absent, "?" injoignable, sinon Ko/Mo.
QString sizeCell(int64_t size, bool known, bool reachable) {
    if (!reachable) return QStringLiteral("?");
    if (!known)     return QStringLiteral("—");
    const double ko = size / 1024.0;
    if (ko < 1024.0) return QString::number(ko, 'f', ko < 10 ? 1 : 0) + " Ko";
    return QString::number(ko / 1024.0, 'f', 1) + " Mo";
}

// Couleur d'accent d'un état (fond doux). Vide = pas d'accent.
QColor accent(FileState s) {
    switch (s) {
        case FileState::UpToDate:              return QColor();                 // neutre
        case FileState::CacheBehind:
        case FileState::MissingFromCache:      return QColor(255, 214, 165, 60);// orange doux
        case FileState::CollectorBehind:
        case FileState::MissingFromCollector:  return QColor(255, 236, 179, 50);// jaune doux
        case FileState::UnexplainedDivergence: return QColor(255, 138, 128, 70);// rouge doux
        case FileState::SourceUnreachable:
        case FileState::CollectorUnreachable:  return QColor(176, 190, 197, 45);// gris doux
    }
    return QColor();
}

} // namespace

CoherenceDialog::CoherenceDialog(const Config& config, const QString& readUrl,
                                 const QMap<QString, QString>& collectors, QWidget* parent)
    : QDialog(parent), config_(config), readUrl_(readUrl), collectors_(collectors) {
    setWindowTitle("Contrôle de cohérence");
    resize(920, 560);

    auto* root = new QVBoxLayout(this);

    header_ = new QLabel;
    header_->setWordWrap(true);
    root->addWidget(header_);

    table_ = new QTableWidget(0, 7, this);
    table_->setHorizontalHeaderLabels(
        {"Site", "Fichier", "o2switch", "Collecteur", "Cache", "État", "Action"});
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int c : {0, 2, 3, 4, 5, 6})
        table_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    root->addWidget(table_, 1);

    status_ = new QLabel("Aucun contrôle effectué.");
    status_->setProperty("muted", true);
    root->addWidget(status_);

    auto* buttons = new QHBoxLayout;
    verifyBtn_ = new QPushButton("Vérifier");
    verifyBtn_->setToolTip("Confronte o2switch, le collecteur et le cache (métadonnées seulement).");
    connect(verifyBtn_, &QPushButton::clicked, this, &CoherenceDialog::runChecks);
    auto* closeBtn = new QPushButton("Fermer");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(verifyBtn_);
    buttons->addStretch();
    buttons->addWidget(closeBtn);
    root->addLayout(buttons);

    // En-tête : source de lecture + éventuels autres collecteurs (information, sans
    // mélange : le contrôle ne porte que sur le collecteur sélectionné).
    QString h = readUrl_.isEmpty()
        ? QStringLiteral("<b>Aucun collecteur sélectionné.</b> Le contrôle comparera "
                         "o2switch et le cache local seulement.")
        : QStringLiteral("Collecteur de lecture : <b>%1</b>").arg(readUrl_.toHtmlEscaped());
    if (collectors_.size() > 1) {
        QStringList others;
        for (auto it = collectors_.constBegin(); it != collectors_.constEnd(); ++it)
            if (it.key() != readUrl_) others << it.key();
        if (!others.isEmpty())
            h += QStringLiteral("<br><span>Autres collecteurs vus (non lus ici) : %1</span>")
                     .arg(others.join(", ").toHtmlEscaped());
    }
    header_->setText(h);

    // Premier contrôle à l'ouverture.
    runChecks();
}

const SiteConfig* CoherenceDialog::siteByName(const std::string& name) const {
    for (const SiteConfig& s : config_.sites)
        if (s.name == name) return &s;
    return nullptr;
}

void CoherenceDialog::runChecks() {
    if (config_.sites.empty()) {
        status_->setText("Aucun site configuré.");
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    verifyBtn_->setEnabled(false);
    reports_.clear();
    for (const SiteConfig& s : config_.sites) {
        status_->setText("Contrôle : " + QString::fromStdString(s.name) + " …");
        QApplication::processEvents();
        reports_.push_back(coherence::checkSite(s, readUrl_, config_.cacheRoot));
    }
    populate();
    verifyBtn_->setEnabled(true);
    QApplication::restoreOverrideCursor();
    status_->setText("Dernier contrôle : " +
                     QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss"));
}

void CoherenceDialog::populate() {
    table_->setRowCount(0);
    int actionable = 0;

    for (const SiteReport& rep : reports_) {
        for (const FileRow& r : rep.rows) {
            const int row = table_->rowCount();
            table_->insertRow(row);

            auto put = [&](int col, const QString& text) {
                auto* it = new QTableWidgetItem(text);
                const QColor a = accent(r.state);
                if (a.isValid()) it->setBackground(a);
                table_->setItem(row, col, it);
            };
            put(0, QString::fromStdString(rep.site));
            put(1, QString::fromStdString(r.name));
            put(2, sizeCell(r.sourceSize,    r.sourceKnown,    rep.sourceReachable));
            put(3, sizeCell(r.collectorSize, r.collectorKnown, rep.collectorReachable));
            put(4, sizeCell(r.cacheSize,     r.cacheKnown,     true));
            QString stateText = coherence::toString(r.state);
            if (!r.note.empty()) stateText += "  (" + QString::fromStdString(r.note) + ")";
            put(5, stateText);

            if (coherence::isActionable(r.state)) ++actionable;

            // Cellule d'action : boutons selon l'état (déclenchés par l'utilisateur).
            auto* cell = new QWidget;
            auto* cl = new QHBoxLayout(cell);
            cl->setContentsMargins(2, 2, 2, 2);
            cl->setSpacing(4);
            const std::string site = rep.site, file = r.name;

            const bool cacheProblem = r.state == FileState::MissingFromCache ||
                                      r.state == FileState::CacheBehind;
            const bool collectorProblem = r.state == FileState::MissingFromCollector ||
                                          r.state == FileState::CollectorBehind;

            if (cacheProblem) {
                auto oid = rep.collectorObjectIds.find(file);
                if (oid != rep.collectorObjectIds.end() && !oid->second.empty()) {
                    const std::string objectId = oid->second;
                    auto* b = new QPushButton("Depuis collecteur");
                    b->setToolTip("Recopie la copie conservée par le collecteur dans le cache.");
                    connect(b, &QPushButton::clicked, this,
                            [this, site, file, objectId] { fillFromCollector(site, file, objectId); });
                    cl->addWidget(b);
                }
                if (r.sourceKnown) {
                    auto* b = new QPushButton("En direct");
                    b->setToolTip("Télécharge le fichier directement depuis o2switch.");
                    connect(b, &QPushButton::clicked, this,
                            [this, site, file] { downloadDirect(site, file); });
                    cl->addWidget(b);
                }
            } else if (collectorProblem) {
                auto* b = new QPushButton("Relancer la collecte");
                b->setToolTip("Demande au collecteur de récupérer ce site maintenant.");
                connect(b, &QPushButton::clicked, this,
                        [this, site] { collectNow(site); });
                cl->addWidget(b);
            } else if (r.state == FileState::UnexplainedDivergence && r.sourceKnown) {
                auto* b = new QPushButton("En direct");
                b->setToolTip("Réaligne le cache sur la source o2switch.");
                connect(b, &QPushButton::clicked, this,
                        [this, site, file] { downloadDirect(site, file); });
                cl->addWidget(b);
            }
            cl->addStretch();
            // Hauteur mini des boutons : sans cela, la ligne par défaut est trop
            // basse et le libellé (« Relancer la collecte ») est tronqué.
            for (QPushButton* b : cell->findChildren<QPushButton*>())
                b->setMinimumHeight(26);
            table_->setCellWidget(row, 6, cell);
        }
    }

    // Ajuste la hauteur des lignes au contenu (les cellules-boutons notamment).
    table_->resizeRowsToContents();

    if (reports_.empty()) return;
    if (actionable == 0)
        status_->setText(status_->text() + "  —  tout est cohérent.");
    else
        status_->setText(status_->text() + QString("  —  %1 ligne(s) à traiter.").arg(actionable));
}

void CoherenceDialog::fillFromCollector(const std::string& site, const std::string& file,
                                        const std::string& objectId) {
    bool ok = false;
    QString err;
    const QByteArray bytes =
        CollectorClient::download(readUrl_, QString::fromStdString(objectId), ok, err);
    if (!ok) {
        QMessageBox::warning(this, "Depuis collecteur",
            "Téléchargement impossible : " + err.toHtmlEscaped());
        return;
    }
    CacheManager cache(config_.cacheRoot);
    const std::string local = cache.siteDir(site) + "/" + file;
    std::ofstream out(local, std::ios::binary | std::ios::trunc);
    if (!out) {
        QMessageBox::warning(this, "Depuis collecteur", "Écriture du cache impossible.");
        return;
    }
    out.write(bytes.constData(), bytes.size());
    out.close();
    runChecks();
}

void CoherenceDialog::downloadDirect(const std::string& site, const std::string& file) {
    const SiteConfig* s = siteByName(site);
    if (!s) return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    SftpClient client;
    std::string err;
    if (!client.connect(*s, err)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, "Téléchargement direct",
            "Connexion SFTP impossible : " + QString::fromStdString(err).toHtmlEscaped());
        return;
    }
    CacheManager cache(config_.cacheRoot);
    const std::string local = cache.siteDir(site) + "/" + file;
    const std::string remote = s->remoteLogDir + "/" + file;
    const bool ok = client.download(remote, local, err);
    client.disconnect();
    QApplication::restoreOverrideCursor();
    if (!ok) {
        QMessageBox::warning(this, "Téléchargement direct",
            "Échec : " + QString::fromStdString(err).toHtmlEscaped());
        return;
    }
    runChecks();
}

void CoherenceDialog::collectNow(const std::string& site) {
    const SiteConfig* s = siteByName(site);
    if (!s || readUrl_.isEmpty()) return;
    const CollectorClient::Reply r =
        CollectorClient::collectNow(readUrl_, QString::fromStdString(s->id));
    if (r.status == 202 || r.ok())
        QMessageBox::information(this, "Collecte",
            "Collecte demandée au collecteur. Relancez « Vérifier » dans quelques instants.");
    else
        QMessageBox::warning(this, "Collecte",
            "Le collecteur a refusé (HTTP " + QString::number(r.status) + ").");
}
