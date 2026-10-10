/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QtGlobal>

#include "core/model/Stats.h"

// Memoire d'autorite WEB de SiteWatch : les series quotidiennes de chaque site,
// conservees sur toute la duree des logs en cache (pas seulement la periode
// affichee). C'est la source de verite que SiteWatch publie vers morfAnalytics ;
// elle ne depend ni de morfCollector ni de morfAnalytics (SiteWatch reste autonome).
class WebAuthorityStore {
public:
    explicit WebAuthorityStore(QString dbPath);
    ~WebAuthorityStore();

    bool open();
    bool isOpen() const { return open_; }
    QString lastError() const { return lastError_; }

    // `files` : nom de fichier de log -> taille. Vrai si un fichier est nouveau ou a
    // change de taille depuis la derniere consolidation (un .gz mensuel grossit :
    // la taille, pas la date, fait foi).
    bool needsConsolidation(const QString& siteId, const QMap<QString, qint64>& files) const;

    // Remplace les jours du site par ceux de `full` (analyse de TOUS les logs, sans
    // filtre de date) et memorise les fichiers vus. Idempotent.
    void consolidate(const QString& siteId, const Stats& full,
                     const QMap<QString, qint64>& files);

    // serie ("daily_humans", ...) -> { "AAAA-MM-JJ": n }. Vide si site inconnu.
    QJsonObject daily(const QString& siteId) const;
    qint64 consolidatedAt(const QString& siteId) const;   // epoch, 0 si jamais
    QString lastDay(const QString& siteId) const;         // dernier jour connu

private:
    QString dbPath_;
    QString lastError_;
    bool open_ = false;
    QString connectionName_;
    QSqlDatabase db_;   // garde le handle (Qt ferme la connexion sinon)
};
