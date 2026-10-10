/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "webstore/WebAuthorityStore.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <utility>

namespace {
// Series conservees : (nom publie, membre de Stats).
using Series = std::pair<const char*, std::map<std::string, long> Stats::*>;
const Series kSeries[] = {
    {"daily_humans",  &Stats::dailyHumans},
    {"daily_bots",    &Stats::dailyBots},
    {"daily_ai",      &Stats::dailyAI},
    {"daily_seo",     &Stats::dailySEO},
    {"daily_404",     &Stats::daily404},
    {"daily_403",     &Stats::daily403},
    {"daily_500",     &Stats::daily500},
    {"daily_attacks", &Stats::dailyAttacks},
    {"daily_normal",  &Stats::dailyNormal},
};
} // namespace

WebAuthorityStore::WebAuthorityStore(QString dbPath)
    : dbPath_(std::move(dbPath)),
      connectionName_(QStringLiteral("sitewatch-web-%1")
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {}

WebAuthorityStore::~WebAuthorityStore() {
    if (db_.isValid() && db_.isOpen())
        db_.close();
    db_ = QSqlDatabase();
    if (QSqlDatabase::contains(connectionName_))
        QSqlDatabase::removeDatabase(connectionName_);
}

bool WebAuthorityStore::open() {
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
    const char* ddl[] = {
        "CREATE TABLE IF NOT EXISTS web_daily ("
        "  site_id TEXT NOT NULL, serie TEXT NOT NULL, day TEXT NOT NULL,"
        "  value INTEGER NOT NULL, PRIMARY KEY(site_id, serie, day))",
        "CREATE TABLE IF NOT EXISTS web_file ("
        "  site_id TEXT NOT NULL, name TEXT NOT NULL, size INTEGER NOT NULL,"
        "  consolidated_at INTEGER NOT NULL, PRIMARY KEY(site_id, name))",
    };
    for (const char* sql : ddl) {
        if (!q.exec(QLatin1String(sql))) {
            lastError_ = q.lastError().text();
            return false;
        }
    }
    // Version du schema : quand les series conservees changent, les fichiers deja vus
    // doivent etre relus une fois pour remplir les nouvelles (sinon rien ne declenche
    // la reconsolidation tant que les logs ne grossissent pas).
    constexpr int kSeriesVersion = 2;   // 2 : + daily_403, daily_500
    q.exec(QStringLiteral("PRAGMA user_version"));
    const int have = q.next() ? q.value(0).toInt() : 0;
    if (have < kSeriesVersion) {
        q.exec(QStringLiteral("DELETE FROM web_file"));
        q.exec(QStringLiteral("PRAGMA user_version=%1").arg(kSeriesVersion));
    }
    open_ = true;
    return true;
}

bool WebAuthorityStore::needsConsolidation(const QString& siteId,
                                           const QMap<QString, qint64>& files) const {
    if (!open_)
        return false;
    QMap<QString, qint64> known;
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT name, size FROM web_file WHERE site_id=?"));
    q.addBindValue(siteId);
    q.exec();
    while (q.next())
        known.insert(q.value(0).toString(), q.value(1).toLongLong());
    for (auto it = files.begin(); it != files.end(); ++it) {
        // Inegalite, pas « plus grand » : une rotation peut produire un fichier plus petit.
        if (!known.contains(it.key()) || known.value(it.key()) != it.value())
            return true;
    }
    return false;
}

void WebAuthorityStore::consolidate(const QString& siteId, const Stats& full,
                                    const QMap<QString, qint64>& files) {
    if (!open_)
        return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    db_.transaction();
    // Les logs en cache forment l'integralite de ce que SiteWatch sait du site :
    // on remplace donc ses jours d'un bloc plutot que de fusionner.
    QSqlQuery del(db_);
    del.prepare(QStringLiteral("DELETE FROM web_daily WHERE site_id=?"));
    del.addBindValue(siteId);
    del.exec();
    for (const Series& s : kSeries) {
        for (const auto& [day, n] : full.*(s.second)) {
            QSqlQuery ins(db_);
            ins.prepare(QStringLiteral(
                "INSERT OR REPLACE INTO web_daily(site_id, serie, day, value) VALUES(?,?,?,?)"));
            ins.addBindValue(siteId);
            ins.addBindValue(QString::fromLatin1(s.first));
            ins.addBindValue(QString::fromStdString(day));
            ins.addBindValue(static_cast<qint64>(n));
            ins.exec();
        }
    }
    QSqlQuery delF(db_);
    delF.prepare(QStringLiteral("DELETE FROM web_file WHERE site_id=?"));
    delF.addBindValue(siteId);
    delF.exec();
    for (auto it = files.begin(); it != files.end(); ++it) {
        QSqlQuery insF(db_);
        insF.prepare(QStringLiteral(
            "INSERT INTO web_file(site_id, name, size, consolidated_at) VALUES(?,?,?,?)"));
        insF.addBindValue(siteId);
        insF.addBindValue(it.key());
        insF.addBindValue(it.value());
        insF.addBindValue(now);
        insF.exec();
    }
    if (!db_.commit())
        lastError_ = db_.lastError().text();
}

QJsonObject WebAuthorityStore::daily(const QString& siteId) const {
    QJsonObject out;
    if (!open_)
        return out;
    QSqlQuery q(db_);
    q.prepare(QStringLiteral(
        "SELECT serie, day, value FROM web_daily WHERE site_id=? ORDER BY day"));
    q.addBindValue(siteId);
    q.exec();
    while (q.next()) {
        const QString serie = q.value(0).toString();
        QJsonObject days = out.value(serie).toObject();
        days.insert(q.value(1).toString(), static_cast<double>(q.value(2).toLongLong()));
        out.insert(serie, days);
    }
    return out;
}

qint64 WebAuthorityStore::consolidatedAt(const QString& siteId) const {
    if (!open_)
        return 0;
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT MAX(consolidated_at) FROM web_file WHERE site_id=?"));
    q.addBindValue(siteId);
    q.exec();
    return q.next() ? q.value(0).toLongLong() : 0;
}

QString WebAuthorityStore::lastDay(const QString& siteId) const {
    if (!open_)
        return {};
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT MAX(day) FROM web_daily WHERE site_id=?"));
    q.addBindValue(siteId);
    q.exec();
    return q.next() ? q.value(0).toString() : QString();
}
