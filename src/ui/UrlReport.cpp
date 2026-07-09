/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/UrlReport.h"

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

#include <utility>
#include <vector>
#include <algorithm>

namespace {

using Row = std::pair<QString, long>;

std::vector<Row> byValue(const std::map<QString, long>& m) {
    std::vector<Row> v(m.begin(), m.end());
    std::sort(v.begin(), v.end(), [](const Row& a, const Row& b) { return a.second > b.second; });
    return v;
}
std::vector<Row> byKey(const std::map<QString, long>& m) {
    return {m.begin(), m.end()};   // ordre chronologique (heures, dates)
}

// Champ CSV international : separateur virgule, guillemets si necessaire (RFC 4180).
QString csvField(const QString& s) {
    if (s.contains(',') || s.contains('"') || s.contains('\n')) {
        QString v = s; v.replace("\"", "\"\"");
        return "\"" + v + "\"";
    }
    return s;
}

} // namespace

urlreport::Aggregate urlreport::aggregate(const std::vector<const LogEntry*>& entries) {
    Aggregate a;
    for (const LogEntry* e : entries) {
        if (!e) continue;
        ++a.total;
        a.ips[QString::fromStdString(e->ip)]++;
        a.uas[QString::fromStdString(e->userAgent.empty() ? "-" : e->userAgent)]++;
        a.status[QString::number(e->status)]++;
        a.urls[QString::fromStdString(e->url)]++;
        const QString r = (e->referer.empty() || e->referer == "-")
                              ? QStringLiteral("Direct") : QString::fromStdString(e->referer);
        a.refs[r]++;
        if (e->hour >= 0) a.hours[QString::asprintf("%02dh", e->hour)]++;
        a.dates[QString::fromStdString(e->date)]++;
    }
    return a;
}

QString urlreport::deaccent(const QString& s) {
    const QString d = s.normalized(QString::NormalizationForm_D);
    QString r;
    for (const QChar& c : d)
        if (c.category() != QChar::Mark_NonSpacing) r += c;
    return r;
}

QString urlreport::groupsText(const std::vector<Group>& groups) {
    QString out;
    for (const Group& g : groups) {
        const Aggregate a = aggregate(g.entries);
        out += "=== " + g.label + " ===\n";
        out += QString::number(a.total) + " requête(s)\n";
        auto sec = [&](const QString& name, const std::vector<Row>& rows) {
            out += "\n[" + name + "]\n";
            for (const auto& r : rows) out += r.first + "\t" + QString::number(r.second) + "\n";
        };
        sec("Adresses IP", byValue(a.ips));
        sec("Codes HTTP", byValue(a.status));
        sec("User-Agents", byValue(a.uas));
        sec("URLs", byValue(a.urls));
        sec("Référents", byValue(a.refs));
        sec("Répartition horaire", byKey(a.hours));
        sec("Évolution (par jour)", byKey(a.dates));
        out += "\n";
    }
    return out;
}

QString urlreport::groupsCsv(const std::vector<Group>& groups) {
    // Une ligne de synthese par groupe. Format international : virgule, sans accents.
    QString out = "Label,Requests,Distinct IPs,Distinct UAs,Distinct URLs,HTTP codes,"
                  "Top IP,Top User-Agent,Top URL,Top referer,First day,Last day\n";

    auto top = [](const std::map<QString, long>& m) -> QString {
        const auto v = byValue(m);
        if (v.empty()) return "-";
        return v.front().first + " (" + QString::number(v.front().second) + ")";
    };

    for (const Group& g : groups) {
        const Aggregate a = aggregate(g.entries);

        QString codes;   // ex. "403 (1200) 200 (45)"
        for (const auto& r : byValue(a.status)) {
            if (!codes.isEmpty()) codes += " ";
            codes += r.first + " (" + QString::number(r.second) + ")";
        }
        const QString firstDay = a.dates.empty() ? "-" : a.dates.begin()->first;
        const QString lastDay  = a.dates.empty() ? "-" : a.dates.rbegin()->first;

        out += csvField(deaccent(g.label)) + ","
             + QString::number(a.total) + ","
             + QString::number(static_cast<int>(a.ips.size())) + ","
             + QString::number(static_cast<int>(a.uas.size())) + ","
             + QString::number(static_cast<int>(a.urls.size())) + ","
             + csvField(codes) + ","
             + csvField(top(a.ips)) + ","
             + csvField(top(a.uas)) + ","
             + csvField(top(a.urls)) + ","
             + csvField(top(a.refs)) + ","
             + csvField(firstDay) + ","
             + csvField(lastDay) + "\n";
    }
    return out;
}

bool urlreport::saveCsv(QWidget* parent, const QString& suggestedName, const QString& csvContent) {
    const QString fn = QFileDialog::getSaveFileName(parent, "Exporter en CSV",
                                                    suggestedName, "Fichier CSV (*.csv)");
    if (fn.isEmpty()) return false;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(parent, "Export", "Écriture impossible : " + fn);
        return false;
    }
    QTextStream out(&f);   // UTF-8 sans BOM (format CSV international)
    out << csvContent;
    f.close();
    return true;
}
