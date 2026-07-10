/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/CacheCleanupDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDate>

namespace {

QString humanSize(qint64 bytes) {
    const double mo = bytes / (1024.0 * 1024.0);
    if (mo >= 1.0) return QString::number(mo, 'f', 1).replace('.', ',') + " Mo";
    return QString::number(bytes / 1024.0, 'f', 1).replace('.', ',') + " Ko";
}

// Extrait le mois d'un nom de fichier o2switch "...-Mon-AAAA.gz" (1er du mois).
QDate fileMonth(const QString& filename) {
    static const QRegularExpression re("-([A-Za-z]{3})-(\\d{4})\\.gz$");
    const auto m = re.match(filename);
    if (!m.hasMatch()) return QDate();
    static const char* names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int month = 0;
    for (int i = 0; i < 12; ++i)
        if (m.captured(1).compare(names[i], Qt::CaseInsensitive) == 0) { month = i + 1; break; }
    if (month == 0) return QDate();
    return QDate(m.captured(2).toInt(), month, 1);
}

} // namespace

CacheCleanupDialog::CacheCleanupDialog(const QString& cacheRoot, const QStringList& siteNames,
                                       QWidget* parent)
    : QDialog(parent), cacheRoot_(cacheRoot), siteNames_(siteNames) {
    setWindowTitle("Effacer des logs téléchargés — SiteWatch");
    setMinimumWidth(560);
    buildUi();
    updateMode();
}

void CacheCleanupDialog::buildUi() {
    auto* root = new QVBoxLayout(this);

    // --- Filtres ---
    auto* form = new QFormLayout;
    siteCombo_ = new QComboBox;
    siteCombo_->addItem("Tous les sites", QString());
    for (const QString& s : siteNames_) siteCombo_->addItem(s, s);
    form->addRow("Site :", siteCombo_);

    modeCombo_ = new QComboBox;
    modeCombo_->addItems({"Tous les fichiers", "Antérieurs à un mois", "Sur une période"});
    form->addRow("Étendue :", modeCombo_);

    dateFrom_ = new QDateEdit(QDate::currentDate());
    dateFrom_->setDisplayFormat("MMMM yyyy");
    dateFrom_->setCalendarPopup(true);
    dateFromLbl_ = new QLabel("Avant le mois :");
    form->addRow(dateFromLbl_, dateFrom_);

    dateTo_ = new QDateEdit(QDate::currentDate());
    dateTo_->setDisplayFormat("MMMM yyyy");
    dateTo_->setCalendarPopup(true);
    dateToLbl_ = new QLabel("Jusqu'au mois :");
    form->addRow(dateToLbl_, dateTo_);
    root->addLayout(form);

    // --- Liste des fichiers concernés ---
    fileList_ = new QListWidget;
    root->addWidget(fileList_);

    auto* checkRow = new QHBoxLayout;
    auto* checkAll = new QPushButton("Tout cocher");
    auto* uncheckAll = new QPushButton("Tout décocher");
    checkRow->addWidget(checkAll);
    checkRow->addWidget(uncheckAll);
    checkRow->addStretch();
    summary_ = new QLabel;
    checkRow->addWidget(summary_);
    root->addLayout(checkRow);

    // --- Boutons ---
    auto* buttons = new QDialogButtonBox;
    deleteBtn_ = buttons->addButton("Supprimer", QDialogButtonBox::AcceptRole);
    deleteBtn_->setObjectName("dangerButton");
    buttons->addButton("Fermer", QDialogButtonBox::RejectRole);
    root->addWidget(buttons);

    connect(siteCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CacheCleanupDialog::refreshList);
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CacheCleanupDialog::updateMode);
    connect(dateFrom_, &QDateEdit::dateChanged, this, &CacheCleanupDialog::refreshList);
    connect(dateTo_,   &QDateEdit::dateChanged, this, &CacheCleanupDialog::refreshList);
    connect(fileList_, &QListWidget::itemChanged, this, &CacheCleanupDialog::updateSummary);
    connect(checkAll, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < fileList_->count(); ++i)
            fileList_->item(i)->setCheckState(Qt::Checked);
    });
    connect(uncheckAll, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < fileList_->count(); ++i)
            fileList_->item(i)->setCheckState(Qt::Unchecked);
    });
    connect(deleteBtn_, &QPushButton::clicked, this, &CacheCleanupDialog::onDelete);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CacheCleanupDialog::updateMode() {
    const int m = modeCombo_->currentIndex();
    dateFromLbl_->setVisible(m >= 1);
    dateFrom_->setVisible(m >= 1);
    dateToLbl_->setVisible(m == 2);
    dateTo_->setVisible(m == 2);
    dateFromLbl_->setText(m == 1 ? "Avant le mois :" : "Du mois :");
    refreshList();
}

void CacheCleanupDialog::refreshList() {
    fileList_->clear();
    const QString site = siteCombo_->currentData().toString();  // vide = tous
    const int mode = modeCombo_->currentIndex();
    const QDate d1(dateFrom_->date().year(), dateFrom_->date().month(), 1);
    const QDate d2(dateTo_->date().year(), dateTo_->date().month(), 1);

    if (cacheRoot_.isEmpty() || !QDir(cacheRoot_).exists()) { updateSummary(); return; }

    QDirIterator it(cacheRoot_, QStringList{"*.gz"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (!site.isEmpty() && fi.dir().dirName() != site) continue;
        if (mode != 0) {
            const QDate fd = fileMonth(fi.fileName());
            if (!fd.isValid()) continue;                       // date illisible : on ne touche pas
            if (mode == 1 && !(fd < d1)) continue;             // antérieurs au mois d1
            if (mode == 2 && (fd < d1 || fd > d2)) continue;   // période [d1, d2]
        }
        auto* item = new QListWidgetItem(
            QString("%1  (%2)").arg(fi.fileName(), humanSize(fi.size())));
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setData(Qt::UserRole + 1, static_cast<qlonglong>(fi.size()));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        fileList_->addItem(item);
    }
    updateSummary();
}

void CacheCleanupDialog::updateSummary() {
    int n = 0;
    qint64 bytes = 0;
    for (int i = 0; i < fileList_->count(); ++i) {
        const auto* it = fileList_->item(i);
        if (it->checkState() == Qt::Checked) {
            ++n;
            bytes += it->data(Qt::UserRole + 1).toLongLong();
        }
    }
    summary_->setText(QString("%1 fichier(s) — %2").arg(n).arg(humanSize(bytes)));
    if (deleteBtn_) deleteBtn_->setEnabled(n > 0);
}

void CacheCleanupDialog::onDelete() {
    QStringList paths;
    qint64 bytes = 0;
    for (int i = 0; i < fileList_->count(); ++i) {
        const auto* it = fileList_->item(i);
        if (it->checkState() == Qt::Checked) {
            paths << it->data(Qt::UserRole).toString();
            bytes += it->data(Qt::UserRole + 1).toLongLong();
        }
    }
    if (paths.isEmpty()) return;

    const auto answer = QMessageBox::warning(
        this, "Confirmation",
        QString("Supprimer définitivement %1 fichier(s) (%2) ?\nCette action est irréversible.")
            .arg(paths.size()).arg(humanSize(bytes)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    int ok = 0, fail = 0;
    for (const QString& p : paths)
        if (QFile::remove(p)) ++ok; else ++fail;

    QMessageBox::information(this, "Suppression",
        QString("%1 fichier(s) supprimé(s)%2.")
            .arg(ok).arg(fail ? QString(", %1 échec(s)").arg(fail) : QString()));
    refreshList();
}
