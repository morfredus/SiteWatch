/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <map>
#include <vector>
#include "core/model/LogEntry.h"

class QWidget;

// -----------------------------------------------------------------------------
// urlreport : agrège et met en forme un sous-ensemble d'entrées analysées —
// pour le détail au double-clic, le presse-papier et l'export CSV. Le même
// moteur sert à toutes les lignes de tous les onglets (URL, référent,
// catégorie d'attaque, activité WordPress…).
// -----------------------------------------------------------------------------
namespace urlreport {

// Agrégat statistique d'un sous-ensemble d'entrées.
struct Aggregate {
    long total = 0;
    std::map<QString, long> ips, uas, status, urls, refs, hours, dates;
};
Aggregate aggregate(const std::vector<const LogEntry*>& entries);

// Un groupe nommé d'entrées : une URL, un référent, une catégorie, etc.
struct Group {
    QString label;
    std::vector<const LogEntry*> entries;
};

// Texte lisible (presse-papier) : une section détaillée par groupe.
QString groupsText(const std::vector<Group>& groups);

// CSV international : une ligne de synthèse par groupe (virgule, sans accents).
QString groupsCsv(const std::vector<Group>& groups);

// Boîte « Enregistrer sous… » + écriture UTF-8 (sans BOM) du contenu CSV fourni.
bool saveCsv(QWidget* parent, const QString& suggestedName, const QString& csvContent);

// Retire les accents d'une chaîne (pour libellés CSV internationaux).
QString deaccent(const QString& s);

} // namespace urlreport
