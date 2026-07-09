/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <QString>
#include <utility>
#include <vector>
#include "core/model/LogEntry.h"

class QTableWidget;
class QGroupBox;
class QPoint;

// -----------------------------------------------------------------------------
// DetailDialog : détail générique d'une ligne (double-clic sur un onglet).
//
// À partir d'un sous-ensemble d'entrées déjà filtrées (une URL, un référent,
// une catégorie d'attaque, une activité WordPress…), affiche les IP, codes
// HTTP, User-Agents, URLs, référents, répartition horaire et évolution.
// Le site concerné est rappelé dans la barre d'info. Chaque tableau propose au
// clic droit : copier / exporter en CSV la sélection ; deux boutons exportent
// la totalité (presse-papier ou CSV).
// -----------------------------------------------------------------------------
class DetailDialog : public QDialog {
    Q_OBJECT
public:
    DetailDialog(const QString& title, const QString& site,
                 const std::vector<const LogEntry*>& matches, QWidget* parent = nullptr);

private:
    QGroupBox* makeSection(const QString& title,
                           const std::vector<std::pair<QString, long>>& rows);
    void showTableMenu(QTableWidget* table, const QString& section, const QPoint& pos);
    void copySelection(QTableWidget* table, const QString& section);
    void exportSelection(QTableWidget* table, const QString& section);

    QString title_;
    QString reportText_;   // toutes les infos (presse-papier)
    QString reportCsv_;    // synthèse (CSV)
};
