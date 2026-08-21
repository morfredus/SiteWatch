/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "github/GitHubTrafficClient.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr auto kApiBase = "https://api.github.com";
constexpr auto kAccept  = "application/vnd.github+json";
constexpr auto kApiVer  = "2022-11-28";
constexpr auto kAgent   = "SiteWatch-github/1";
constexpr int  kTimeoutMs = 25000;

struct HttpResult {
    int status = 0;
    QJsonValue json;
    qint64 rateLimit = -1;
    qint64 rateRemain = -1;
    qint64 rateReset = -1;
    QString error;
};

QString redact(const QString& s) {
    static const QRegularExpression re(
        QStringLiteral("ghp_[A-Za-z0-9_]{10,}|github_pat_[A-Za-z0-9_]{10,}"));
    QString out = s;
    out.replace(re, QStringLiteral("[redacted]"));
    return out;
}

HttpResult getOnce(QNetworkAccessManager& nam, const QUrl& url, const QString& token) {
    HttpResult r;
    QNetworkRequest req(url);
    req.setRawHeader("Accept", kAccept);
    req.setRawHeader("X-GitHub-Api-Version", kApiVer);
    req.setRawHeader("User-Agent", kAgent);
    if (!token.isEmpty())
        req.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());
    QNetworkReply* reply = nam.get(req);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(kTimeoutMs);
    loop.exec();
    if (!timer.isActive() && reply->isRunning()) {
        reply->abort();
        r.error = QStringLiteral("delai depasse");
        reply->deleteLater();
        return r;
    }
    r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    r.rateLimit  = reply->rawHeader("X-RateLimit-Limit").toLongLong();
    r.rateRemain = reply->rawHeader("X-RateLimit-Remaining").toLongLong();
    r.rateReset  = reply->rawHeader("X-RateLimit-Reset").toLongLong();
    if (reply->error() != QNetworkReply::NoError && r.status == 0)
        r.error = redact(reply->errorString());
    reply->deleteLater();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &pe);
    if (pe.error == QJsonParseError::NoError) {
        if (doc.isObject()) r.json = doc.object();
        else if (doc.isArray()) r.json = doc.array();
    }
    return r;
}

QJsonObject endpointStatus(const HttpResult& r) {
    QJsonObject o;
    o[QStringLiteral("status")] = r.status;
    o[QStringLiteral("ok")] = r.status >= 200 && r.status < 300;
    if (!r.error.isEmpty())
        o[QStringLiteral("error")] = redact(r.error);
    else if (r.status >= 400 && r.json.isObject()) {
        const QString msg = r.json.toObject().value(QStringLiteral("message")).toString();
        if (!msg.isEmpty())
            o[QStringLiteral("error")] = redact(msg);
    }
    return o;
}

QJsonArray fetchReleases(QNetworkAccessManager& nam, const QString& owner,
                         const QString& repo, const QString& token, HttpResult& last) {
    QJsonArray all;
    for (int page = 1; page <= 20; ++page) {
        QUrl url(QStringLiteral("%1/repos/%2/%3/releases")
                     .arg(QLatin1String(kApiBase), owner, repo));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("per_page"), QStringLiteral("100"));
        q.addQueryItem(QStringLiteral("page"), QString::number(page));
        url.setQuery(q);
        last = getOnce(nam, url, token);
        if (last.status < 200 || last.status >= 300 || !last.json.isArray())
            break;
        const QJsonArray pageArr = last.json.toArray();
        if (pageArr.isEmpty())
            break;
        for (const QJsonValue& v : pageArr)
            all.append(v);
        if (pageArr.size() < 100)
            break;
    }
    return all;
}

} // namespace

GitHubTrafficClient::Result GitHubTrafficClient::fetch(const QString& owner,
                                                       const QString& repo,
                                                       const QString& token) {
    Result res;
    if (token.trimmed().isEmpty()) {
        res.error = QStringLiteral("jeton GitHub absent");
        return res;
    }
    QNetworkAccessManager nam;
    const QString prefix = QStringLiteral("%1/repos/%2/%3")
                               .arg(QLatin1String(kApiBase), owner, repo);
    const HttpResult repoR = getOnce(nam, QUrl(prefix), token);
    if (repoR.status == 401 || repoR.status == 403) {
        res.error = QStringLiteral("acces GitHub refuse (jeton absent, expire ou droits insuffisants)");
        return res;
    }
    if (repoR.status == 404) {
        res.error = QStringLiteral("depot introuvable ou non visible avec ce jeton");
        return res;
    }
    if (repoR.status == 0) {
        res.error = repoR.error.isEmpty() ? QStringLiteral("GitHub injoignable") : repoR.error;
        return res;
    }

    const HttpResult viewsR  = getOnce(nam, QUrl(prefix + QStringLiteral("/traffic/views")), token);
    const HttpResult clonesR = getOnce(nam, QUrl(prefix + QStringLiteral("/traffic/clones")), token);
    const HttpResult pathsR  = getOnce(nam, QUrl(prefix + QStringLiteral("/traffic/popular/paths")), token);
    const HttpResult refsR   = getOnce(nam, QUrl(prefix + QStringLiteral("/traffic/popular/referrers")), token);
    HttpResult relR;
    const QJsonArray releases = fetchReleases(nam, owner, repo, token, relR);

    bool anyOk = repoR.status >= 200 && repoR.status < 300;
    int okCount = anyOk ? 1 : 0;
    const HttpResult* const extras[] = {&viewsR, &clonesR, &pathsR, &refsR, &relR};
    for (const HttpResult* e : extras) {
        if (e->status >= 200 && e->status < 300) {
            ++okCount;
            anyOk = true;
        }
    }
    if (!anyOk) {
        res.error = QStringLiteral("aucun endpoint GitHub n'a repondu correctement");
        return res;
    }

    const QDate utc = QDateTime::currentDateTimeUtc().date();
    QJsonObject period{
        {QStringLiteral("kind"), QStringLiteral("github_rolling_14d")},
        {QStringLiteral("days"), 14},
        {QStringLiteral("from"), utc.addDays(-13).toString(Qt::ISODate)},
        {QStringLiteral("to"), utc.toString(Qt::ISODate)},
    };
    QJsonObject data;
    if (repoR.json.isObject())
        data[QStringLiteral("repository")] = repoR.json.toObject();
    if (viewsR.json.isObject())
        data[QStringLiteral("views")] = viewsR.json.toObject();
    if (clonesR.json.isObject())
        data[QStringLiteral("clones")] = clonesR.json.toObject();
    if (pathsR.json.isArray())
        data[QStringLiteral("popular_paths")] = pathsR.json.toArray();
    if (refsR.json.isArray())
        data[QStringLiteral("referrers")] = refsR.json.toArray();
    if (relR.status >= 200 && relR.status < 300)
        data[QStringLiteral("releases")] = releases;

    QJsonArray diagnostics;
    if (okCount < 6)
        diagnostics.append(QStringLiteral("reponse partielle"));
    if (viewsR.status == 403 || clonesR.status == 403)
        diagnostics.append(QStringLiteral(
            "trafic refuse : le jeton n'a probablement pas le droit Administration (lecture)"));

    QJsonObject endpoints;
    endpoints[QStringLiteral("repo")] = endpointStatus(repoR);
    endpoints[QStringLiteral("views")] = endpointStatus(viewsR);
    endpoints[QStringLiteral("clones")] = endpointStatus(clonesR);
    endpoints[QStringLiteral("paths")] = endpointStatus(pathsR);
    endpoints[QStringLiteral("referrers")] = endpointStatus(refsR);
    endpoints[QStringLiteral("releases")] = endpointStatus(relR);

    res.snapshot = QJsonObject{
        {QStringLiteral("contract"), QStringLiteral("github-traffic/1")},
        {QStringLiteral("collected_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("owner"), owner},
        {QStringLiteral("repository"), repo},
        {QStringLiteral("full_name"), owner + QLatin1Char('/') + repo},
        {QStringLiteral("period"), period},
        {QStringLiteral("endpoints"), endpoints},
        {QStringLiteral("partial"), okCount < 6},
        {QStringLiteral("data"), data},
        {QStringLiteral("diagnostics"), diagnostics},
        {QStringLiteral("rate_limit"), QJsonObject{
            {QStringLiteral("limit"), static_cast<double>(repoR.rateLimit)},
            {QStringLiteral("remaining"), static_cast<double>(repoR.rateRemain)},
            {QStringLiteral("reset"), static_cast<double>(repoR.rateReset)},
        }},
    };
    res.ok = true;
    return res;
}
