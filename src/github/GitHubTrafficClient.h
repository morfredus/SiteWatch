/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QJsonObject>
#include <QString>

// Interroge l'API GitHub pour un depot suivi. SiteWatch en est proprietaire :
// morfCollector n'est pas requis pour la premiere mesure. Le jeton n'est jamais
// recopie dans le snapshot.
class GitHubTrafficClient {
public:
    struct Result {
        bool ok = false;
        QString error;
        QJsonObject snapshot;
    };

    static Result fetch(const QString& owner, const QString& repo, const QString& token);
};
