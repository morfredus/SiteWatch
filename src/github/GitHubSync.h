/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>
#include <atomic>

#include "config/Config.h"
#include "github/GitHubAuthorityStore.h"

// Rattrapage et publication GitHub SANS interface : partage entre la page GitHub de la
// GUI et l'outil headless (sitewatch-publish).
namespace githubsync {

// Lit (RESEAU seulement, sans base : utilisable dans un fil de travail) les instantanes
// archives par morfCollector pour les depots suivis.
QVector<QJsonObject> fetchSnapshots(const Config& cfg, const QString& collectorUrl,
                                    const std::atomic<bool>* cancel = nullptr);

// Verse des instantanes dans le magasin d'autorite (fil de l'interface : la connexion
// SQLite appartient a ce fil). Retourne le nombre de jours REELLEMENT nouveaux.
int ingest(GitHubAuthorityStore& store, const QVector<QJsonObject>& snapshots);

// fetchSnapshots + ingest, pour l'outil console.
int catchUp(const Config& cfg, const QString& collectorUrl, GitHubAuthorityStore& store);

// Publie la verite consolidee vers morfAnalytics (POST /github/ingest). `ok` dit si
// la publication a abouti ; le texte retourne decrit le resultat dans tous les cas.
QString publish(const QString& analyticsUrl, GitHubAuthorityStore& store, bool& ok);

} // namespace githubsync
