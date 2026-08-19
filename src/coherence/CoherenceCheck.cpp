/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "coherence/CoherenceCheck.h"

#include "collector/CollectorClient.h"
#include "core/net/SftpClient.h"
#include "core/net/LogDiscovery.h"

#include <QJsonArray>
#include <QJsonObject>

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace coherence {

namespace {

// Vue o2switch : listage SFTP filtré au site. reachable=false si la connexion ou
// le listage échoue (impossibilité de vérifier, jamais une absence).
PathView sourceView(const SiteConfig& site, std::string& error) {
    PathView v;
    SftpClient client;
    std::string err;
    if (!client.connect(site, err)) {
        error = err.empty() ? "connexion SFTP impossible" : err;
        return v;   // reachable reste false
    }
    const auto files = client.listLogs(site.remoteLogDir, err);
    if (files.empty() && !err.empty()) {
        error = err;
        client.disconnect();
        return v;   // dossier illisible : injoignable, pas « vide »
    }
    for (const auto& f : files) {
        if (!logdiscovery::belongsToSite(f.name, site.name, site.logMatch))
            continue;
        v.sizes[f.name] = static_cast<int64_t>(f.size);
    }
    v.reachable = true;   // listage abouti (même si aucun fichier ne correspond)
    client.disconnect();
    return v;
}

// Vue collecteur : GET /objects (métadonnées). On distingue le transport (aucune
// réponse => injoignable) d'un 404 « rien pour cette source » (joignable, vide).
PathView collectorView(const SiteConfig& site, const QString& baseUrl, std::string& error,
                       std::map<std::string, std::string>& objectIds) {
    PathView v;
    if (baseUrl.isEmpty()) {
        error = "aucun collecteur sélectionné";
        return v;
    }
    const CollectorClient::Reply r =
        CollectorClient::getObjects(baseUrl, QString::fromStdString(site.id));
    if (r.status == 0) {   // pas de réponse HTTP = transport en échec
        error = r.error.isEmpty() ? "collecteur injoignable" : r.error.toStdString();
        return v;
    }
    v.reachable = true;   // le collecteur a répondu (200 = objets ; 404 = rien)
    if (r.ok()) {
        for (const QJsonValue& val : r.json.value(QStringLiteral("objects")).toArray()) {
            const QJsonObject o = val.toObject();
            const QString name = o.value(QStringLiteral("original_name")).toString();
            if (name.isEmpty())
                continue;
            v.sizes[name.toStdString()] =
                static_cast<int64_t>(o.value(QStringLiteral("size")).toDouble());
            objectIds[name.toStdString()] =
                o.value(QStringLiteral("object_id")).toString().toStdString();
        }
    }
    // status non-2xx autre que « source inconnue » : on garde reachable=true mais
    // on note l'anomalie (le moteur verra une vue vide côté collecteur).
    else if (r.status != 404) {
        error = "collecteur : HTTP " + std::to_string(r.status);
    }
    return v;
}

// Vue cache : les .gz présents localement pour ce site (toujours joignable).
PathView cacheView(const SiteConfig& site, const std::string& cacheRoot) {
    PathView v;
    v.reachable = true;
    const fs::path dir = fs::path(cacheRoot) / site.name;
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
        return v;   // pas encore de cache pour ce site : joignable, vide
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec))
            continue;
        const std::string name = entry.path().filename().string();
        if (name.size() < 3 || name.substr(name.size() - 3) != ".gz")
            continue;
        if (!logdiscovery::belongsToSite(name, site.name, site.logMatch))
            continue;   // défensif : le cache ne devrait tenir que ce site
        v.sizes[name] = static_cast<int64_t>(fs::file_size(entry.path(), ec));
    }
    return v;
}

} // namespace

SiteReport checkSite(const SiteConfig& site, const QString& collectorBaseUrl,
                     const std::string& cacheRoot) {
    SiteReport rep;
    rep.site = site.name;

    const PathView src = sourceView(site, rep.sourceError);
    const PathView col = collectorView(site, collectorBaseUrl, rep.collectorError,
                                       rep.collectorObjectIds);
    const PathView cache = cacheView(site, cacheRoot);

    rep.sourceReachable    = src.reachable;
    rep.collectorReachable = col.reachable;
    rep.rows = evaluate(src, col, cache);
    return rep;
}

} // namespace coherence
