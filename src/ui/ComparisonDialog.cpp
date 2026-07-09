/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/ComparisonDialog.h"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLocale>
#include <QFont>

#include <functional>

namespace {

long botOf(const Stats& s, const char* engine) {
    auto it = s.botCounts.find(engine);
    return it != s.botCounts.end() ? it->second : 0L;
}
long attackOf(const Stats& s, const char* label) {
    auto it = s.attackActivity.find(label);
    return it != s.attackActivity.end() ? it->second : 0L;
}

} // namespace

ComparisonDialog::ComparisonDialog(const std::vector<std::pair<QString, Stats>>& sites,
                                   QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Comparaison des sites — SiteWatch");
    resize(760, 560);

    struct Metric {
        QString label;
        std::function<long(const Stats&)> value;
    };
    const std::vector<Metric> metrics = {
        {"Total requêtes", [](const Stats& s) { return s.totalRequests; }},
        {"Humains",        [](const Stats& s) { return s.humans; }},
        {"Robots",         [](const Stats& s) { return s.bots; }},
        {"Google",         [](const Stats& s) { return botOf(s, "Google"); }},
        {"Bing",           [](const Stats& s) { return botOf(s, "Bing"); }},
        {"Claude",         [](const Stats& s) { return botOf(s, "Claude"); }},
        {"OpenAI",         [](const Stats& s) { return botOf(s, "OpenAI"); }},
        {"Semrush",        [](const Stats& s) { return botOf(s, "Semrush"); }},
        {"Ahrefs",         [](const Stats& s) { return botOf(s, "Ahrefs"); }},
        {"Erreurs 404",    [](const Stats& s) { return s.errors404; }},
        {"Erreurs 403",    [](const Stats& s) { return s.errors403; }},
        {"Erreurs 500",    [](const Stats& s) { return s.errors500; }},
        {"XML-RPC",        [](const Stats& s) { return attackOf(s, "XML-RPC"); }},
        {"phpMyAdmin",     [](const Stats& s) { return attackOf(s, "phpMyAdmin"); }},
    };

    const QLocale fr(QLocale::French, QLocale::France);

    auto* table = new QTableWidget;
    table->setColumnCount(1 + static_cast<int>(sites.size()));
    QStringList headers;
    headers << "Indicateur";
    for (const auto& [name, st] : sites) headers << name;
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c <= static_cast<int>(sites.size()); ++c)
        table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);

    table->setRowCount(static_cast<int>(metrics.size()));
    for (int r = 0; r < static_cast<int>(metrics.size()); ++r) {
        auto* label = new QTableWidgetItem(metrics[r].label);
        QFont f = label->font(); f.setBold(true); label->setFont(f);
        table->setItem(r, 0, label);
        for (int c = 0; c < static_cast<int>(sites.size()); ++c) {
            const long v = metrics[r].value(sites[c].second);
            auto* item = new QTableWidgetItem(fr.toString(static_cast<qlonglong>(v)));
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(r, c + 1, item);
        }
    }

    auto* root = new QVBoxLayout(this);
    root->addWidget(table);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText("Fermer");
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}
