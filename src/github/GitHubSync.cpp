/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "github/GitHubSync.h"

#include "collector/CollectorClient.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace githubsync {
namespace {

QByteArray waitReply(QNetworkReply* reply, int timeoutMs) {
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    if (reply->isRunning())
        reply->abort();
    return reply->readAll();
}

QString withoutTrailingSlash(QString url) {
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    return url;
}

} // namespace

QVector<QJsonObject> fetchSnapshots(const Config& cfg, const QString& collectorUrl,
                                    const std::atomic<bool>* cancel) {
    QVector<QJsonObject> out;
    if (collectorUrl.isEmpty())
        return out;
    for (const GitHubRepoConfig& repo : cfg.github.repositories) {
        if (cancel && cancel->load())
            break;
        if (!repo.enabled)
            continue;
        const QString full = QString::fromStdString(cfg.github.owner + "/" + repo.name);
        const QString sid = QStringLiteral("sitewatch:github:") + full;
        const CollectorClient::Reply objs = CollectorClient::getObjects(collectorUrl, sid);
        for (const QJsonValue& v : objs.json.value(QStringLiteral("objects")).toArray()) {
            const QString oid = v.toObject().value(QStringLiteral("object_id")).toString();
            if (oid.isEmpty())
                continue;
            bool ok = false;
            QString err;
            const QByteArray raw = CollectorClient::download(collectorUrl, oid, ok, err);
            if (!ok)
                continue;
            const QJsonObject snap = QJsonDocument::fromJson(raw).object();
            if (snap.value(QStringLiteral("contract")).toString() != QLatin1String("github-traffic/1"))
                continue;
            out.append(snap);
        }
    }
    return out;
}

int ingest(GitHubAuthorityStore& store, const QVector<QJsonObject>& snapshots) {
    if (!store.isOpen())
        return 0;
    int ingested = 0;
    for (const QJsonObject& snap : snapshots)
        ingested += store.ingestSnapshot(snap, QStringLiteral("morfcollector"));
    return ingested;
}

int catchUp(const Config& cfg, const QString& collectorUrl, GitHubAuthorityStore& store) {
    return ingest(store, fetchSnapshots(cfg, collectorUrl));
}

QString publish(const QString& analyticsUrl, GitHubAuthorityStore& store, bool& ok) {
    ok = false;
    const QString base = withoutTrailingSlash(analyticsUrl);
    QNetworkAccessManager nam;

    QNetworkRequest stReq(QUrl(base + QStringLiteral("/status")));
    stReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply* stReply = nam.get(stReq);
    const QJsonObject status = QJsonDocument::fromJson(waitReply(stReply, 8000)).object();
    const int stHttp = stReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    stReply->deleteLater();
    const QString app = status.value(QStringLiteral("app")).toString();
    if (stHttp != 200 || app.isEmpty()) {
        return QStringLiteral(
            "Publication : %1 ne répond pas comme morfAnalytics (HTTP %2). "
            "Vérifiez le port 8799, pas le beacon 8787.").arg(base).arg(stHttp);
    }

    const QByteArray payload =
        QJsonDocument(store.exportAuthority()).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(base + QStringLiteral("/github/ingest")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply* reply = nam.post(req, payload);
    const QByteArray raw = waitReply(reply, 20000);
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonObject body = QJsonDocument::fromJson(raw).object();
    reply->deleteLater();

    if (http == 200 && body.value(QStringLiteral("ok")).toBool()) {
        ok = true;
        return QStringLiteral("Vérité GitHub publiée vers %1 (v%2).")
            .arg(base, status.value(QStringLiteral("version")).toString());
    }
    if (http == 405)
        return QStringLiteral(
            "Publication refusée (HTTP 405) sur %1 : ce morfAnalytics est trop ancien "
            "ou n'expose pas POST /github/ingest. Mettez à jour le service.").arg(base);
    const QString detail = body.value(QStringLiteral("detail")).toString();
    const QString err = body.value(QStringLiteral("error")).toString();
    return QStringLiteral("Publication refusée (HTTP %1) sur %2%3")
        .arg(http)
        .arg(base)
        .arg(err.isEmpty() && detail.isEmpty()
                 ? QString()
                 : QStringLiteral(" : ") + (detail.isEmpty() ? err : detail));
}

} // namespace githubsync
