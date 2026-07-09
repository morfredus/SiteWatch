/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QDialog>
#include <vector>
#include <utility>
#include <QString>
#include "core/model/Stats.h"

// -----------------------------------------------------------------------------
// ComparisonDialog : tableau comparatif de plusieurs sites sur une même période.
// Reçoit la liste (nom de site, statistiques déjà calculées).
// -----------------------------------------------------------------------------
class ComparisonDialog : public QDialog {
    Q_OBJECT
public:
    ComparisonDialog(const std::vector<std::pair<QString, Stats>>& sites,
                     QWidget* parent = nullptr);
};
