/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "collector/CollectorClient.h"

#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QUrl>

namespace {

constexpr quint16 kBeaconUdpPort = 45454;

// Requete HTTP synchrone (bloque via une boucle d'evenements locale).
CollectorClient::Reply httpRequest(const QString& urlStr, const QByteArray& verb,
                                   const QByteArray& body) {
    CollectorClient::Reply r;
    QNetworkAccessManager nam;
    QNetworkRequest req((QUrl(urlStr)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply* reply = nam.sendCustomRequest(req, verb, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(8000);
    loop.exec();

    if (!timer.isActive() && reply->isRunning()) {
        reply->abort();
        r.error = QStringLiteral("delai depasse");
    }
    if (reply->error() != QNetworkReply::NoError && r.error.isEmpty())
        r.error = reply->errorString();

    r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    r.json = QJsonDocument::fromJson(data).object();
    reply->deleteLater();
    return r;
}

CollectorClient::Reply httpGet(const QString& url) {
    return httpRequest(url, "GET", QByteArray());
}
CollectorClient::Reply httpPost(const QString& url, const QJsonObject& body) {
    return httpRequest(url, "POST", QJsonDocument(body).toJson(QJsonDocument::Compact));
}

} // namespace

bool CollectorClient::discover(int timeoutMs, Discovered& out, QString& error) {
    QUdpSocket sock;
    if (!sock.bind(QHostAddress::AnyIPv4, kBeaconUdpPort,
                   QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        error = QStringLiteral("ecoute UDP %1 impossible : %2")
                    .arg(kBeaconUdpPort).arg(sock.errorString());
        return false;
    }

    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < timeoutMs) {
        if (!sock.hasPendingDatagrams()) {
            QEventLoop loop;
            QTimer t;
            t.setSingleShot(true);
            QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
            QObject::connect(&sock, &QUdpSocket::readyRead, &loop, &QEventLoop::quit);
            t.start(static_cast<int>(timeoutMs - clock.elapsed()));
            loop.exec();
        }
        while (sock.hasPendingDatagrams()) {
            QByteArray buf(int(sock.pendingDatagramSize()), '\0');
            QHostAddress sender;
            sock.readDatagram(buf.data(), buf.size(), &sender);

            const QJsonObject o = QJsonDocument::fromJson(buf).object();
            if (!o.value(QStringLiteral("proto")).toString().startsWith(QStringLiteral("morfbeacon/")))
                continue;

            QStringList caps;
            for (const QJsonValue& v : o.value(QStringLiteral("capabilities")).toArray())
                caps << v.toString();
            // Regle du contrat : on cherche la CAPACITE, pas le nom.
            if (!caps.contains(QStringLiteral("collection")))
                continue;

            out.found        = true;
            QString host     = sender.toString();
            if (host.startsWith(QStringLiteral("::ffff:")))   // IPv4 mappee
                host = host.mid(7);
            out.host         = host;
            out.statusPort   = static_cast<quint16>(o.value(QStringLiteral("status_port")).toInt());
            out.app          = o.value(QStringLiteral("app")).toString();
            out.capabilities = caps;
            return true;
        }
    }
    error = QStringLiteral("aucun morfCollector (capacite 'collection') vu en %1 ms").arg(timeoutMs);
    return false;
}

bool CollectorClient::checkCompatible(const QString& baseUrl, const QString& wantedProto,
                                      QJsonObject& status, QString& error) {
    const Reply r = httpGet(baseUrl + QStringLiteral("/status"));
    if (!r.ok()) {
        error = r.error.isEmpty() ? QStringLiteral("/status : HTTP %1").arg(r.status) : r.error;
        return false;
    }
    status = r.json;
    const QJsonArray protos = r.json.value(QStringLiteral("collect"))
                                    .toObject().value(QStringLiteral("proto")).toArray();
    for (const QJsonValue& v : protos)
        if (v.toString() == wantedProto)
            return true;
    error = QStringLiteral("protocole %1 non supporte par le collecteur").arg(wantedProto);
    return false;
}

CollectorClient::Reply CollectorClient::manifestState(const QString& baseUrl, const QString& instance) {
    return httpGet(baseUrl + QStringLiteral("/manifest/state?instance=")
                   + QString::fromUtf8(QUrl::toPercentEncoding(instance)));
}

CollectorClient::Reply CollectorClient::pushManifest(const QString& baseUrl, const QJsonObject& manifest) {
    return httpPost(baseUrl + QStringLiteral("/manifest"), manifest);
}

CollectorClient::Reply CollectorClient::pushCredentials(const QString& baseUrl, const QString& ref,
                                                        const QJsonObject& secret) {
    return httpPost(baseUrl + QStringLiteral("/credentials"),
                    QJsonObject{ {"ref", ref}, {"secret", secret} });
}

CollectorClient::Reply CollectorClient::getSources(const QString& baseUrl) {
    return httpGet(baseUrl + QStringLiteral("/sources"));
}

CollectorClient::Reply CollectorClient::getObjects(const QString& baseUrl, const QString& sourceId) {
    return httpGet(baseUrl + QStringLiteral("/sources/") + sourceId + QStringLiteral("/objects"));
}

QByteArray CollectorClient::download(const QString& baseUrl, const QString& objectId,
                                     bool& ok, QString& error) {
    QNetworkAccessManager nam;
    QNetworkRequest req((QUrl(baseUrl + QStringLiteral("/objects/") + objectId)));
    QNetworkReply* reply = nam.get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(30000);   // un original peut etre volumineux
    loop.exec();

    if (!timer.isActive() && reply->isRunning()) { reply->abort(); error = QStringLiteral("delai depasse"); }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data;
    ok = (reply->error() == QNetworkReply::NoError && status == 200);
    if (ok) data = reply->readAll();
    else if (error.isEmpty()) error = reply->errorString().isEmpty()
                 ? QStringLiteral("HTTP %1").arg(status) : reply->errorString();
    reply->deleteLater();
    return data;
}

CollectorClient::Reply CollectorClient::getStatus(const QString& baseUrl) {
    return httpGet(baseUrl + QStringLiteral("/status"));
}

CollectorClient::Reply CollectorClient::collectNow(const QString& baseUrl, const QString& sourceId) {
    return httpPost(baseUrl + QStringLiteral("/sources/") + sourceId + QStringLiteral("/collect"),
                    QJsonObject{});
}

CollectorClient::Reply CollectorClient::suspend(const QString& baseUrl, const QString& sourceId) {
    return httpPost(baseUrl + QStringLiteral("/sources/") + sourceId + QStringLiteral("/suspend"),
                    QJsonObject{});
}

CollectorClient::Reply CollectorClient::resume(const QString& baseUrl, const QString& sourceId) {
    return httpPost(baseUrl + QStringLiteral("/sources/") + sourceId + QStringLiteral("/resume"),
                    QJsonObject{});
}

CollectorClient::Reply CollectorClient::deleteObject(const QString& baseUrl, const QString& objectId) {
    return httpRequest(baseUrl + QStringLiteral("/objects/") + objectId, "DELETE", QByteArray());
}

CollectorClient::Reply CollectorClient::deleteSourceObjects(const QString& baseUrl, const QString& sourceId) {
    return httpRequest(baseUrl + QStringLiteral("/sources/") + sourceId + QStringLiteral("/objects"),
                       "DELETE", QByteArray());
}

CollectorClient::Reply CollectorClient::deleteSource(const QString& baseUrl, const QString& sourceId) {
    return httpRequest(baseUrl + QStringLiteral("/sources/") + sourceId, "DELETE", QByteArray());
}
