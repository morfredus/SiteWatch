/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QString>
#include <QVector>

// Catalogue des depots d'un compte ou d'une organisation, pour que l'utilisateur
// coche ceux a suivre au lieu de retaper les noms. Le jeton n'est jamais logue.
class GitHubRepoCatalog {
public:
    struct Repo {
        QString name;
        bool isPrivate = false;
        bool archived = false;
    };

    struct Result {
        bool ok = false;
        QString error;
        QVector<Repo> repos;
    };

    // `owner` : login GitHub (compte ou organisation). `token` peut etre vide
    // (liste publique seulement). Pagination jusqu'a epuisement de Link: next.
    static Result listOwned(const QString& owner, const QString& token);
};
