/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <QJsonObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <QtGlobal>

// Memoire d'autorite GitHub de SiteWatch. Une collecte morfCollector ne remplace
// jamais une mesure deja consolidee : elle ne comble que les absences.
class GitHubAuthorityStore {
public:
    struct Row {
        QString fullName;
        QString provenance;
        qint64 lastCollectTs = 0;
        QString lastError;
        qint64 views14 = 0;
        qint64 uniques14 = 0;
        qint64 clones = 0;
        qint64 downloads = 0;
        QString lastRelease;
    };

    explicit GitHubAuthorityStore(QString dbPath);
    ~GitHubAuthorityStore();

    bool open();
    bool isOpen() const { return open_; }
    QString lastError() const { return lastError_; }

    // `provenance` : github-direct | morfcollector
    void ingestSnapshot(const QJsonObject& snap, const QString& provenance);
    Row row(const QString& fullName) const;
    QJsonObject exportAuthority() const;
    bool hasConsolidatedData() const;

private:
    void ingestDaily(const QString& full, const QString& metric, const QJsonObject& block,
                     const QString& provenance);
    void upsertState(const QJsonObject& snap, const QString& provenance);
    void ingestLatestLists(const QString& full, const QJsonObject& data,
                           const QString& provenance);

    QString dbPath_;
    QString lastError_;
    bool open_ = false;
    QString connectionName_;
    // Qt ferme la connexion dès que le dernier QSqlDatabase local est détruit :
    // on garde le handle ici, sinon la base paraît vide au redémarrage.
    QSqlDatabase db_;
};
