/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/DetailDialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QPoint>
#include <QAction>
#include "ui/UrlReport.h"

#include <map>
#include <vector>
#include <algorithm>

namespace {

// Convertit une map en liste triée par valeur décroissante (limitée).
std::vector<std::pair<QString, long>> byCount(const std::map<QString, long>& m, int top = 25) {
    std::vector<std::pair<QString, long>> rows(m.begin(), m.end());
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (top > 0 && static_cast<int>(rows.size()) > top) rows.resize(top);
    return rows;
}

// Lignes sélectionnées (triées) ou, si aucune sélection, toutes les lignes.
QList<int> targetRows(QTableWidget* t) {
    QList<int> rows;
    const auto sel = t->selectionModel()->selectedRows();
    if (!sel.isEmpty())
        for (const auto& idx : sel) rows << idx.row();
    else
        for (int r = 0; r < t->rowCount(); ++r) rows << r;
    std::sort(rows.begin(), rows.end());
    return rows;
}

// Échappement CSV international (séparateur virgule, RFC 4180).
QString csvField(const QString& s) {
    if (s.contains(',') || s.contains('"') || s.contains('\n')) {
        QString v = s; v.replace("\"", "\"\"");
        return "\"" + v + "\"";
    }
    return s;
}

} // namespace

QGroupBox* DetailDialog::makeSection(const QString& title,
                                     const std::vector<std::pair<QString, long>>& rows) {
    auto* box = new QGroupBox(title);
    auto* v = new QVBoxLayout(box);
    v->setContentsMargins(6, 6, 6, 6);

    auto* t = new QTableWidget;
    t->setColumnCount(2);
    t->horizontalHeader()->setVisible(false);
    t->verticalHeader()->setVisible(false);
    t->setShowGrid(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);   // multi, non contiguë
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    t->setToolTip("Clic droit : copier ou exporter la sélection");
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    t->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        t->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        auto* c = new QTableWidgetItem(QString::number(rows[i].second));
        c->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        t->setItem(i, 1, c);
    }
    t->setMinimumHeight(140);

    connect(t, &QWidget::customContextMenuRequested, this,
            [this, t, title](const QPoint& p) { showTableMenu(t, title, p); });

    v->addWidget(t);
    return box;
}

void DetailDialog::showTableMenu(QTableWidget* table, const QString& section, const QPoint& pos) {
    QMenu menu(this);
    QAction* copyAct = menu.addAction("Copier");
    QAction* csvAct  = menu.addAction("Exporter en CSV…");
    QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
    if (chosen == copyAct)      copySelection(table, section);
    else if (chosen == csvAct)  exportSelection(table, section);
}

void DetailDialog::copySelection(QTableWidget* table, const QString& section) {
    QString text = title_ + "\n" + section + "\n";
    for (int r : targetRows(table)) {
        const QString a = table->item(r, 0) ? table->item(r, 0)->text() : QString();
        const QString b = table->item(r, 1) ? table->item(r, 1)->text() : QString();
        text += a + "\t" + b + "\n";
    }
    QApplication::clipboard()->setText(text);
}

void DetailDialog::exportSelection(QTableWidget* table, const QString& section) {
    const QString sec = urlreport::deaccent(section);   // libellés sans accents
    QString csv = "Group,Section,Value,Count\n";
    for (int r : targetRows(table)) {
        const QString a = table->item(r, 0) ? table->item(r, 0)->text() : QString();
        const QString b = table->item(r, 1) ? table->item(r, 1)->text() : QString();
        csv += csvField(urlreport::deaccent(title_)) + "," + csvField(sec) + ","
             + csvField(a) + "," + csvField(b) + "\n";
    }
    urlreport::saveCsv(this, sec + ".csv", csv);
}

DetailDialog::DetailDialog(const QString& title, const QString& site,
                           const std::vector<const LogEntry*>& matches, QWidget* parent)
    : QDialog(parent), title_(title) {
    setWindowTitle("Détail — SiteWatch");
    resize(940, 680);

    const urlreport::Aggregate a = urlreport::aggregate(matches);

    auto* root = new QVBoxLayout(this);

    auto* head = new QLabel("<b>" + title.toHtmlEscaped() + "</b>");
    head->setWordWrap(true);
    head->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(head);

    // Barre d'info : site concerné + volumétrie.
    auto* sub = new QLabel(
        QString("Site : %1   •   %2 requête(s) • %3 IP distincte(s) • %4 User-Agent(s) • %5 URL(s)"
                "   —   clic droit sur un tableau pour copier / exporter")
            .arg(site.isEmpty() ? QStringLiteral("—") : site)
            .arg(a.total).arg(a.ips.size()).arg(a.uas.size()).arg(a.urls.size()));
    sub->setProperty("muted", true);
    sub->setWordWrap(true);
    root->addWidget(sub);

    std::vector<std::pair<QString, long>> hoursOrdered(a.hours.begin(), a.hours.end());
    std::vector<std::pair<QString, long>> datesOrdered(a.dates.begin(), a.dates.end());

    auto* grid = new QGridLayout;
    grid->addWidget(makeSection("Adresses IP", byCount(a.ips)),          0, 0);
    grid->addWidget(makeSection("Codes HTTP", byCount(a.status)),        0, 1);
    grid->addWidget(makeSection("User-Agents", byCount(a.uas)),          1, 0);
    grid->addWidget(makeSection("URLs", byCount(a.urls)),                1, 1);
    grid->addWidget(makeSection("Référents", byCount(a.refs)),           2, 0);
    grid->addWidget(makeSection("Répartition horaire", hoursOrdered),    2, 1);
    grid->addWidget(makeSection("Évolution (par jour)", datesOrdered),   3, 0, 1, 2);
    root->addLayout(grid);

    reportText_ = urlreport::groupsText({{title, matches}});
    reportCsv_  = urlreport::groupsCsv({{title, matches}});

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText("Fermer");
    auto* copyAll   = buttons->addButton("Copier tout", QDialogButtonBox::ActionRole);
    auto* exportAll = buttons->addButton("Exporter tout (CSV)…", QDialogButtonBox::ActionRole);
    connect(copyAll, &QPushButton::clicked, this,
            [this] { QApplication::clipboard()->setText(reportText_); });
    connect(exportAll, &QPushButton::clicked, this,
            [this] { urlreport::saveCsv(this, "detail.csv", reportCsv_); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}
