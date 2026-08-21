/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "github/GitHubRepoCatalog.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace {

constexpr int kTimeoutMs = 15000;
constexpr int kMaxPages = 20;

struct HttpPage {
    int status = 0;
    QString error;
    QByteArray body;
    QByteArray link;
};

void applyGithubHeaders(QNetworkRequest& req, const QString& token) {
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    // GitHub refuse les clients sans User-Agent.
    req.setRawHeader("User-Agent", "SiteWatch");
    if (!token.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + token.toUtf8());
}

HttpPage getPage(const QUrl& url, const QString& token) {
    HttpPage out;
    QNetworkAccessManager nam;
    QNetworkRequest req(url);
    applyGithubHeaders(req, token);

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
        out.error = QStringLiteral("GitHub n'a pas repondu a temps");
    }
    if (reply->error() != QNetworkReply::NoError && out.error.isEmpty())
        out.error = reply->errorString();
    out.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    out.body = reply->readAll();
    out.link = reply->rawHeader("Link");
    reply->deleteLater();
    return out;
}

QUrl nextFromLink(const QByteArray& link) {
    // Ex. <https://api.github.com/user/repos?page=2>; rel="next"
    const QString s = QString::fromUtf8(link);
    static const QRegularExpression re(
        QStringLiteral("<([^>]+)>;\\s*rel=\"next\""));
    const auto m = re.match(s);
    if (!m.hasMatch())
        return {};
    return QUrl(m.captured(1));
}

QString authError(int status) {
    if (status == 401 || status == 403)
        return QStringLiteral(
            "acces GitHub refuse (jeton absent, expire ou droits insuffisants)");
    if (status == 404)
        return QStringLiteral("proprietaire introuvable");
    return QStringLiteral("GitHub a repondu HTTP %1").arg(status);
}

void appendRepos(QVector<GitHubRepoCatalog::Repo>& out, const QByteArray& body,
                 const QString& owner) {
    const QJsonArray arr = QJsonDocument::fromJson(body).array();
    const QString want = owner.toLower();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString login = o.value(QStringLiteral("owner")).toObject()
                                  .value(QStringLiteral("login")).toString();
        if (!login.isEmpty() && login.toLower() != want)
            continue;
        GitHubRepoCatalog::Repo r;
        r.name = o.value(QStringLiteral("name")).toString();
        if (r.name.isEmpty())
            continue;
        r.isPrivate = o.value(QStringLiteral("private")).toBool();
        r.archived = o.value(QStringLiteral("archived")).toBool();
        out.push_back(r);
    }
}

bool fetchPaged(const QUrl& start, const QString& token, const QString& owner,
                QVector<GitHubRepoCatalog::Repo>& out, QString& error) {
    QUrl url = start;
    for (int page = 0; page < kMaxPages && url.isValid(); ++page) {
        const HttpPage p = getPage(url, token);
        if (p.status == 0) {
            error = p.error.isEmpty() ? QStringLiteral("GitHub injoignable") : p.error;
            return false;
        }
        if (p.status < 200 || p.status >= 300) {
            error = authError(p.status);
            return false;
        }
        appendRepos(out, p.body, owner);
        url = nextFromLink(p.link);
    }
    return true;
}

void dedupeSort(QVector<GitHubRepoCatalog::Repo>& repos) {
    std::sort(repos.begin(), repos.end(), [](const auto& a, const auto& b) {
        return a.name.toLower() < b.name.toLower();
    });
    QVector<GitHubRepoCatalog::Repo> unique;
    for (const auto& r : repos) {
        if (unique.isEmpty() || unique.last().name.toLower() != r.name.toLower())
            unique.push_back(r);
    }
    repos.swap(unique);
}

} // namespace

GitHubRepoCatalog::Result GitHubRepoCatalog::listOwned(const QString& owner,
                                                       const QString& token) {
    Result res;
    const QString own = owner.trimmed();
    if (own.isEmpty()) {
        res.error = QStringLiteral("indiquez le proprietaire GitHub");
        return res;
    }

    if (!token.isEmpty()) {
        // Liste authentifiee : comptes + orgs visibles, puis on filtre owner.
        QUrl u(QStringLiteral("https://api.github.com/user/repos"));
        u.setQuery(QStringLiteral(
            "per_page=100&affiliation=owner,organization_member&sort=full_name"));
        if (!fetchPaged(u, token, own, res.repos, res.error))
            return res;
    }

    if (res.repos.isEmpty()) {
        // Public (sans jeton) ou jeton qui ne voit pas ce owner via /user/repos.
        QString ignored;
        QUrl users(QStringLiteral("https://api.github.com/users/%1/repos").arg(own));
        users.setQuery(QStringLiteral("per_page=100&type=owner&sort=full_name"));
        if (!fetchPaged(users, token, own, res.repos, ignored)) {
            QUrl orgs(QStringLiteral("https://api.github.com/orgs/%1/repos").arg(own));
            orgs.setQuery(QStringLiteral("per_page=100&type=all&sort=full_name"));
            if (!fetchPaged(orgs, token, own, res.repos, res.error) && res.repos.isEmpty())
                return res;
            res.error.clear();
        } else {
            QUrl orgs(QStringLiteral("https://api.github.com/orgs/%1/repos").arg(own));
            orgs.setQuery(QStringLiteral("per_page=100&type=all&sort=full_name"));
            QString orgErr;
            fetchPaged(orgs, token, own, res.repos, orgErr);
        }
    }

    dedupeSort(res.repos);
    res.ok = true;
    return res;
}
