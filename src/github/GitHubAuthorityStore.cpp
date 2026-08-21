/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "github/GitHubAuthorityStore.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVector>

GitHubAuthorityStore::GitHubAuthorityStore(QString dbPath)
    : dbPath_(std::move(dbPath)),
      connectionName_(QStringLiteral("sitewatch-github-%1")
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {}

GitHubAuthorityStore::~GitHubAuthorityStore() {
    if (db_.isValid() && db_.isOpen())
        db_.close();
    db_ = QSqlDatabase();
    if (!connectionName_.isEmpty() && QSqlDatabase::contains(connectionName_))
        QSqlDatabase::removeDatabase(connectionName_);
}

bool GitHubAuthorityStore::open() {
    QDir().mkpath(QFileInfo(dbPath_).absolutePath());
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(dbPath_);
    if (!db_.open()) {
        lastError_ = db_.lastError().text();
        return false;
    }
    QSqlQuery q(db_);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    const char* ddl[] = {
        "CREATE TABLE IF NOT EXISTS github_traffic_daily ("
        "  full_name TEXT NOT NULL, metric TEXT NOT NULL, day TEXT NOT NULL,"
        "  count INTEGER, uniques INTEGER, provenance TEXT, collected_at INTEGER,"
        "  PRIMARY KEY(full_name, metric, day))",
        "CREATE TABLE IF NOT EXISTS github_repo_state ("
        "  full_name TEXT PRIMARY KEY, provenance TEXT, last_collect_ts INTEGER,"
        "  last_error TEXT, views_14 INTEGER, uniques_14 INTEGER, clones INTEGER,"
        "  downloads INTEGER, last_release TEXT, stars INTEGER)",
        "CREATE TABLE IF NOT EXISTS github_discrepancies ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT, full_name TEXT, metric TEXT, day TEXT,"
        "  stored_count INTEGER, incoming_count INTEGER, stored_prov TEXT,"
        "  incoming_prov TEXT, logged_at INTEGER)",
        // Listes glissantes 14 j et assets : un seul jeu "dernier connu" par depot.
        "CREATE TABLE IF NOT EXISTS github_popular_paths ("
        "  full_name TEXT NOT NULL, rank INTEGER NOT NULL, path TEXT,"
        "  count INTEGER, uniques INTEGER, PRIMARY KEY(full_name, rank))",
        "CREATE TABLE IF NOT EXISTS github_referrers ("
        "  full_name TEXT NOT NULL, rank INTEGER NOT NULL, referrer TEXT,"
        "  count INTEGER, uniques INTEGER, PRIMARY KEY(full_name, rank))",
        "CREATE TABLE IF NOT EXISTS github_release_assets ("
        "  full_name TEXT NOT NULL, asset_id INTEGER NOT NULL, name TEXT,"
        "  download_count INTEGER, PRIMARY KEY(full_name, asset_id))",
    };
    for (const char* sql : ddl) {
        if (!q.exec(QLatin1String(sql))) {
            lastError_ = q.lastError().text();
            return false;
        }
    }
    // Bases deja ouvertes avant 1.17.0 : la colonne stars n'existait pas.
    q.exec(QStringLiteral("ALTER TABLE github_repo_state ADD COLUMN stars INTEGER"));
    open_ = true;
    return true;
}

void GitHubAuthorityStore::ingestDaily(const QString& full, const QString& metric,
                                       const QJsonObject& block, const QString& provenance) {
    QSqlDatabase db = db_;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    // GitHub met les series journalieres dans views[] / clones[] selon le nom du bloc.
    const QString seriesKey = metric;
    for (const QJsonValue& v : block.value(seriesKey).toArray()) {
        const QJsonObject d = v.toObject();
        const QString day = d.value(QStringLiteral("timestamp")).toString().left(10);
        if (day.size() < 10)
            continue;
        const qint64 count = static_cast<qint64>(d.value(QStringLiteral("count")).toDouble());
        const qint64 uniques = static_cast<qint64>(d.value(QStringLiteral("uniques")).toDouble());
        QSqlQuery sel(db);
        sel.prepare(QStringLiteral(
            "SELECT count, uniques, provenance FROM github_traffic_daily "
            "WHERE full_name=? AND metric=? AND day=?"));
        sel.addBindValue(full);
        sel.addBindValue(metric);
        sel.addBindValue(day);
        sel.exec();
        if (sel.next()) {
            const qint64 haveC = sel.value(0).toLongLong();
            const qint64 haveU = sel.value(1).toLongLong();
            const QString haveP = sel.value(2).toString();
            if (haveC == count && haveU == uniques)
                continue;
            QSqlQuery disc(db);
            disc.prepare(QStringLiteral(
                "INSERT INTO github_discrepancies(full_name, metric, day, stored_count, "
                "incoming_count, stored_prov, incoming_prov, logged_at) "
                "VALUES(?,?,?,?,?,?,?,?)"));
            disc.addBindValue(full);
            disc.addBindValue(metric);
            disc.addBindValue(day);
            disc.addBindValue(haveC);
            disc.addBindValue(count);
            disc.addBindValue(haveP);
            disc.addBindValue(provenance);
            disc.addBindValue(now);
            disc.exec();
            continue;
        }
        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT INTO github_traffic_daily(full_name, metric, day, count, uniques, "
            "provenance, collected_at) VALUES(?,?,?,?,?,?,?)"));
        ins.addBindValue(full);
        ins.addBindValue(metric);
        ins.addBindValue(day);
        ins.addBindValue(count);
        ins.addBindValue(uniques);
        ins.addBindValue(provenance);
        ins.addBindValue(now);
        ins.exec();
    }
}

void GitHubAuthorityStore::upsertState(const QJsonObject& snap, const QString& provenance) {
    const QString full = snap.value(QStringLiteral("full_name")).toString();
    QSqlDatabase db = db_;
    QSqlQuery sel(db);
    sel.prepare(QStringLiteral(
        "SELECT provenance, last_collect_ts FROM github_repo_state WHERE full_name=?"));
    sel.addBindValue(full);
    sel.exec();
    const bool exists = sel.next();
    const QString haveP = exists ? sel.value(0).toString() : QString();
    // Une archive collecteur ne recouvre pas un etat deja pose par GitHub direct.
    if (exists && haveP == QLatin1String("github-direct")
        && provenance == QLatin1String("morfcollector"))
        return;

    const QJsonObject data = snap.value(QStringLiteral("data")).toObject();
    const QJsonObject views = data.value(QStringLiteral("views")).toObject();
    const QJsonObject clones = data.value(QStringLiteral("clones")).toObject();
    qint64 downloads = 0;
    QString lastRel;
    for (const QJsonValue& rel : data.value(QStringLiteral("releases")).toArray()) {
        const QJsonObject ro = rel.toObject();
        if (lastRel.isEmpty())
            lastRel = ro.value(QStringLiteral("tag_name")).toString();
        for (const QJsonValue& a : ro.value(QStringLiteral("assets")).toArray())
            downloads += static_cast<qint64>(
                a.toObject().value(QStringLiteral("download_count")).toDouble());
    }
    const qint64 ts = QDateTime::currentSecsSinceEpoch();
    const qint64 stars = static_cast<qint64>(
        data.value(QStringLiteral("repository")).toObject()
            .value(QStringLiteral("stargazers_count")).toDouble());
    QSqlQuery u(db);
    u.prepare(QStringLiteral(
        "INSERT INTO github_repo_state(full_name, provenance, last_collect_ts, last_error, "
        "views_14, uniques_14, clones, downloads, last_release, stars) "
        "VALUES(?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(full_name) DO UPDATE SET provenance=excluded.provenance, "
        "last_collect_ts=excluded.last_collect_ts, last_error=excluded.last_error, "
        "views_14=excluded.views_14, uniques_14=excluded.uniques_14, "
        "clones=excluded.clones, downloads=excluded.downloads, "
        "last_release=excluded.last_release, stars=excluded.stars"));
    u.addBindValue(full);
    u.addBindValue(provenance);
    u.addBindValue(ts);
    u.addBindValue(QString());
    u.addBindValue(static_cast<qint64>(views.value(QStringLiteral("count")).toDouble()));
    u.addBindValue(static_cast<qint64>(views.value(QStringLiteral("uniques")).toDouble()));
    u.addBindValue(static_cast<qint64>(clones.value(QStringLiteral("count")).toDouble()));
    u.addBindValue(downloads);
    u.addBindValue(lastRel);
    u.addBindValue(stars);
    u.exec();
}

void GitHubAuthorityStore::ingestSnapshot(const QJsonObject& snap, const QString& provenance) {
    if (!open_)
        return;
    const QString full = snap.value(QStringLiteral("full_name")).toString();
    if (full.isEmpty())
        return;
    const QJsonObject data = snap.value(QStringLiteral("data")).toObject();
    ingestDaily(full, QStringLiteral("views"), data.value(QStringLiteral("views")).toObject(),
                provenance);
    ingestDaily(full, QStringLiteral("clones"), data.value(QStringLiteral("clones")).toObject(),
                provenance);
    upsertState(snap, provenance);
    ingestLatestLists(full, data, provenance);
}

bool GitHubAuthorityStore::hasConsolidatedData() const {
    if (!open_)
        return false;
    QSqlQuery q(db_);
    q.exec(QStringLiteral("SELECT 1 FROM github_repo_state LIMIT 1"));
    return q.next();
}

void GitHubAuthorityStore::ingestLatestLists(const QString& full, const QJsonObject& data,
                                             const QString& provenance) {
    QSqlDatabase db = db_;
    QSqlQuery has(db);
    has.prepare(QStringLiteral("SELECT 1 FROM github_popular_paths WHERE full_name=? LIMIT 1"));
    has.addBindValue(full);
    has.exec();
    // Une archive collecteur ne recouvre pas les listes deja posees par GitHub.
    if (has.next() && provenance == QLatin1String("morfcollector"))
        return;

    QSqlQuery del(db);
    del.prepare(QStringLiteral("DELETE FROM github_popular_paths WHERE full_name=?"));
    del.addBindValue(full);
    del.exec();
    del.prepare(QStringLiteral("DELETE FROM github_referrers WHERE full_name=?"));
    del.addBindValue(full);
    del.exec();
    del.prepare(QStringLiteral("DELETE FROM github_release_assets WHERE full_name=?"));
    del.addBindValue(full);
    del.exec();

    int rank = 0;
    for (const QJsonValue& v : data.value(QStringLiteral("popular_paths")).toArray()) {
        const QJsonObject o = v.toObject();
        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT INTO github_popular_paths(full_name, rank, path, count, uniques) "
            "VALUES(?,?,?,?,?)"));
        ins.addBindValue(full);
        ins.addBindValue(rank++);
        ins.addBindValue(o.value(QStringLiteral("path")).toString());
        ins.addBindValue(static_cast<qint64>(o.value(QStringLiteral("count")).toDouble()));
        ins.addBindValue(static_cast<qint64>(o.value(QStringLiteral("uniques")).toDouble()));
        ins.exec();
    }
    rank = 0;
    for (const QJsonValue& v : data.value(QStringLiteral("referrers")).toArray()) {
        const QJsonObject o = v.toObject();
        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT INTO github_referrers(full_name, rank, referrer, count, uniques) "
            "VALUES(?,?,?,?,?)"));
        ins.addBindValue(full);
        ins.addBindValue(rank++);
        ins.addBindValue(o.value(QStringLiteral("referrer")).toString());
        ins.addBindValue(static_cast<qint64>(o.value(QStringLiteral("count")).toDouble()));
        ins.addBindValue(static_cast<qint64>(o.value(QStringLiteral("uniques")).toDouble()));
        ins.exec();
    }
    for (const QJsonValue& rel : data.value(QStringLiteral("releases")).toArray()) {
        for (const QJsonValue& a : rel.toObject().value(QStringLiteral("assets")).toArray()) {
            const QJsonObject o = a.toObject();
            const qint64 id = static_cast<qint64>(o.value(QStringLiteral("id")).toDouble());
            if (id <= 0)
                continue;
            QSqlQuery ins(db);
            ins.prepare(QStringLiteral(
                "INSERT INTO github_release_assets(full_name, asset_id, name, download_count) "
                "VALUES(?,?,?,?) ON CONFLICT(full_name, asset_id) DO UPDATE SET "
                "name=excluded.name, download_count=excluded.download_count"));
            ins.addBindValue(full);
            ins.addBindValue(id);
            ins.addBindValue(o.value(QStringLiteral("name")).toString());
            ins.addBindValue(static_cast<qint64>(
                o.value(QStringLiteral("download_count")).toDouble()));
            ins.exec();
        }
    }
}

GitHubAuthorityStore::Row GitHubAuthorityStore::row(const QString& fullName) const {
    Row r;
    r.fullName = fullName;
    if (!open_)
        return r;
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "SELECT provenance, last_collect_ts, last_error, views_14, uniques_14, clones, "
        "downloads, last_release FROM github_repo_state WHERE full_name=?"));
    q.addBindValue(fullName);
    q.exec();
    if (!q.next())
        return r;
    r.provenance = q.value(0).toString();
    r.lastCollectTs = q.value(1).toLongLong();
    r.lastError = q.value(2).toString();
    r.views14 = q.value(3).toLongLong();
    r.uniques14 = q.value(4).toLongLong();
    r.clones = q.value(5).toLongLong();
    r.downloads = q.value(6).toLongLong();
    r.lastRelease = q.value(7).toString();
    return r;
}

QJsonObject GitHubAuthorityStore::exportAuthority() const {
    QJsonObject out;
    out[QStringLiteral("contract")] = QStringLiteral("sitewatch-github/1");
    out[QStringLiteral("published_at")] =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!open_)
        return out;
    QSqlDatabase db = db_;
    // SQLite n'aime pas deux QSqlQuery actives sur la meme connexion : on lit
    // d'abord les depots, puis les series et listes depot par depot.
    struct RepoOut {
        QString full;
        QString provenance;
        qint64 lastCollectTs = 0;
        qint64 views14 = 0;
        qint64 uniques14 = 0;
        qint64 clones = 0;
        qint64 downloads = 0;
        QString lastRelease;
        qint64 stars = 0;
    };
    QVector<RepoOut> list;
    QSqlQuery st(db);
    st.exec(QStringLiteral(
        "SELECT full_name, provenance, last_collect_ts, views_14, uniques_14, clones, "
        "downloads, last_release, stars FROM github_repo_state"));
    while (st.next()) {
        RepoOut r;
        r.full = st.value(0).toString();
        r.provenance = st.value(1).toString();
        r.lastCollectTs = st.value(2).toLongLong();
        r.views14 = st.value(3).toLongLong();
        r.uniques14 = st.value(4).toLongLong();
        r.clones = st.value(5).toLongLong();
        r.downloads = st.value(6).toLongLong();
        r.lastRelease = st.value(7).toString();
        r.stars = st.value(8).toLongLong();
        list.append(r);
    }
    QJsonArray repos;
    for (const RepoOut& r : list) {
        QJsonObject repo;
        repo[QStringLiteral("full_name")] = r.full;
        repo[QStringLiteral("provenance")] = r.provenance;
        repo[QStringLiteral("last_collect_ts")] = static_cast<double>(r.lastCollectTs);
        repo[QStringLiteral("views_14")] = static_cast<double>(r.views14);
        repo[QStringLiteral("uniques_14")] = static_cast<double>(r.uniques14);
        repo[QStringLiteral("clones")] = static_cast<double>(r.clones);
        repo[QStringLiteral("downloads")] = static_cast<double>(r.downloads);
        repo[QStringLiteral("last_release")] = r.lastRelease;
        repo[QStringLiteral("stars")] = static_cast<double>(r.stars);
        QJsonArray daily;
        QSqlQuery d(db);
        d.prepare(QStringLiteral(
            "SELECT metric, day, count, uniques, provenance FROM github_traffic_daily "
            "WHERE full_name=? ORDER BY day"));
        d.addBindValue(r.full);
        d.exec();
        while (d.next()) {
            daily.append(QJsonObject{
                {QStringLiteral("metric"), d.value(0).toString()},
                {QStringLiteral("day"), d.value(1).toString()},
                {QStringLiteral("count"), static_cast<double>(d.value(2).toLongLong())},
                {QStringLiteral("uniques"), static_cast<double>(d.value(3).toLongLong())},
                {QStringLiteral("provenance"), d.value(4).toString()},
            });
        }
        repo[QStringLiteral("daily")] = daily;

        QJsonArray paths;
        d.prepare(QStringLiteral(
            "SELECT path, count, uniques FROM github_popular_paths "
            "WHERE full_name=? ORDER BY rank"));
        d.addBindValue(r.full);
        d.exec();
        while (d.next()) {
            paths.append(QJsonObject{
                {QStringLiteral("path"), d.value(0).toString()},
                {QStringLiteral("count"), static_cast<double>(d.value(1).toLongLong())},
                {QStringLiteral("uniques"), static_cast<double>(d.value(2).toLongLong())},
            });
        }
        repo[QStringLiteral("popular_paths")] = paths;

        QJsonArray refs;
        d.prepare(QStringLiteral(
            "SELECT referrer, count, uniques FROM github_referrers "
            "WHERE full_name=? ORDER BY rank"));
        d.addBindValue(r.full);
        d.exec();
        while (d.next()) {
            refs.append(QJsonObject{
                {QStringLiteral("referrer"), d.value(0).toString()},
                {QStringLiteral("count"), static_cast<double>(d.value(1).toLongLong())},
                {QStringLiteral("uniques"), static_cast<double>(d.value(2).toLongLong())},
            });
        }
        repo[QStringLiteral("referrers")] = refs;

        QJsonArray assets;
        d.prepare(QStringLiteral(
            "SELECT asset_id, name, download_count FROM github_release_assets "
            "WHERE full_name=?"));
        d.addBindValue(r.full);
        d.exec();
        while (d.next()) {
            assets.append(QJsonObject{
                {QStringLiteral("asset_id"), static_cast<double>(d.value(0).toLongLong())},
                {QStringLiteral("name"), d.value(1).toString()},
                {QStringLiteral("download_count"),
                 static_cast<double>(d.value(2).toLongLong())},
            });
        }
        repo[QStringLiteral("assets")] = assets;
        repos.append(repo);
    }
    out[QStringLiteral("repositories")] = repos;
    return out;
}
