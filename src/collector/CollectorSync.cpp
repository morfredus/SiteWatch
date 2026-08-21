/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "collector/CollectorSync.h"
#include "collector/CollectorClient.h"

#include <QCryptographicHash>
#include <QSysInfo>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QSet>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace collectorsync {
namespace {

QString providerInstance() {
    return QStringLiteral("SiteWatch@") + QSysInfo::machineHostName();
}

QJsonObject buildWebManifest(const Config& cfg, const QString& instance,
                             const QString& generation, qint64 revision) {
    // Collecte quotidienne à l'heure configurée (défaut 02:00, cf. contrat §1.2.1).
    const QString dailyAt = cfg.collectorDailyAt.empty()
        ? QStringLiteral("02:00") : QString::fromStdString(cfg.collectorDailyAt);
    const QJsonObject schedule{ {"daily_at", dailyAt} };

    QJsonArray sources;
    for (const SiteConfig& s : cfg.sites) {
        sources.append(QJsonObject{
            {"source_id", QString::fromStdString(s.id)},
            {"label", QString::fromStdString(s.name)},
            {"enabled", true},
            {"connector", QJsonObject{ {"name", "sftp"}, {"version", 1} }},
            {"params", QJsonObject{
                {"host", QString::fromStdString(s.host)},
                {"remote_dir", QString::fromStdString(s.remoteLogDir)},
                {"match", QString::fromStdString(s.logMatch)},
                {"site_key", QString::fromStdString(s.name)},
                {"source_key", QStringLiteral("sitewatch:web:")
                    + QString::fromStdString(s.domain.empty() ? s.name : s.domain)},
            }},
            {"credentials_ref", QStringLiteral("sw-") + QString::fromStdString(s.id)},
            {"schedule", schedule},
            {"retention", QJsonObject{ {"mode", "keep_forever"} }},
        });
    }
    return QJsonObject{
        {"proto", "morfcollect/1"},
        {"provider", QJsonObject{ {"app", "SiteWatch"}, {"instance", instance} }},
        {"manifest_generation", generation},
        {"revision", static_cast<double>(revision)},
        {"sources", sources},
    };
}

QString githubProviderInstance() {
    return QStringLiteral("SiteWatch.github@") + QSysInfo::machineHostName();
}

QString githubSourceId(const Config& cfg, const GitHubRepoConfig& repo) {
    return QStringLiteral("sitewatch:github:")
        + QString::fromStdString(cfg.github.owner) + QLatin1Char('/')
        + QString::fromStdString(repo.name);
}

QString githubCredentialsRef(const Config& cfg) {
    return QStringLiteral("sw-github-") + QString::fromStdString(cfg.github.owner);
}

QJsonObject buildGithubManifest(const Config& cfg, const QString& instance,
                                const QString& generation, qint64 revision) {
    const QString dailyAt = !cfg.github.dailyAt.empty()
        ? QString::fromStdString(cfg.github.dailyAt)
        : (cfg.collectorDailyAt.empty()
               ? QStringLiteral("02:30") : QString::fromStdString(cfg.collectorDailyAt));
    const QJsonObject schedule{ {"daily_at", dailyAt} };

    QJsonArray sources;
    if (cfg.github.enabled && !cfg.github.owner.empty()) {
        for (const GitHubRepoConfig& r : cfg.github.repositories) {
            // Decoche = non suivi : absent du manifeste, pas une source suspendue.
            if (!r.enabled)
                continue;
            sources.append(QJsonObject{
                {"source_id", githubSourceId(cfg, r)},
                {"label", QString::fromStdString(cfg.github.owner + "/" + r.name)},
                {"enabled", r.enabled},
                {"connector", QJsonObject{ {"name", "github-traffic"}, {"version", 1} }},
                {"params", QJsonObject{
                    {"owner", QString::fromStdString(cfg.github.owner)},
                    {"repository", QString::fromStdString(r.name)},
                }},
                {"credentials_ref", githubCredentialsRef(cfg)},
                {"schedule", schedule},
                {"retention", QJsonObject{ {"mode", "keep_forever"} }},
            });
        }
    }
    return QJsonObject{
        {"proto", "morfcollect/1"},
        {"provider", QJsonObject{ {"app", "SiteWatch"}, {"instance", instance} }},
        {"manifest_generation", generation},
        {"revision", static_cast<double>(revision)},
        {"sources", sources},
    };
}

QJsonObject buildSecret(const SiteConfig& s) {
    QJsonObject secret;
    if (!s.user.empty())     secret["user"]     = QString::fromStdString(s.user);
    if (!s.password.empty()) secret["password"] = QString::fromStdString(s.password);
    if (!s.keyFile.empty())  secret["key_file"] = QString::fromStdString(s.keyFile);
    return secret;
}

QString sourcesHash(const QJsonObject& manifest) {
    const QByteArray data = QJsonDocument(manifest.value("sources").toArray())
                                .toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
}

struct State {
    QString     generation;
    qint64      revision = 0;
    QString     hash;
    QStringList sourceIds;      // ids de la derniere synchro (pour ajouts/retraits)
    QJsonObject labelsById;     // id -> label (pour libeller un retrait)
};

QString statePath(const QString& configPath) { return configPath + QStringLiteral(".collector.json"); }

State loadState(const QString& configPath) {
    State st;
    QFile f(statePath(configPath));
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        st.generation = o.value("manifest_generation").toString();
        st.revision   = static_cast<qint64>(o.value("revision").toDouble());
        st.hash       = o.value("hash").toString();
        st.labelsById = o.value("labels_by_id").toObject();
        for (const QJsonValue& v : o.value("source_ids").toArray())
            st.sourceIds << v.toString();
    }
    return st;
}

void saveState(const QString& configPath, const State& st) {
    QJsonArray ids;
    for (const QString& id : st.sourceIds) ids.append(id);
    QFile f(statePath(configPath));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(QJsonObject{
            {"manifest_generation", st.generation},
            {"revision", static_cast<double>(st.revision)},
            {"hash", st.hash},
            {"source_ids", ids},
            {"labels_by_id", st.labelsById},
        }).toJson(QJsonDocument::Indented));
    }
}

} // namespace

QString locate(const QString& explicitUrl, int discoverTimeoutMs) {
    if (!explicitUrl.isEmpty())
        return explicitUrl;
    CollectorClient::Discovered d;
    QString err;
    if (CollectorClient::discover(discoverTimeoutMs, d, err))
        return d.baseUrl();
    return QString();
}

SyncResult synchronize(const QString& baseUrl, const Config& cfg,
                       const QString& configPath, bool forceCredentials) {
    SyncResult res;

    // 1. Compatibilite du protocole.
    QJsonObject status; QString cerr;
    if (!CollectorClient::checkCompatible(baseUrl, QStringLiteral("morfcollect/1"), status, cerr)) {
        res.error = cerr;
        return res;
    }
    res.metrics = status.value(QStringLiteral("metrics")).toObject();

    // 2. Etat local + ajouts/retraits par rapport a la derniere synchro.
    State st = loadState(configPath);
    if (st.generation.isEmpty()) { st.generation = QUuid::createUuid().toString(QUuid::WithoutBraces); st.revision = 0; }

    QSet<QString> prevIds(st.sourceIds.begin(), st.sourceIds.end());
    QSet<QString> curIds;
    QStringList   curIdList;
    QJsonObject   curLabels;
    for (const SiteConfig& s : cfg.sites) {
        const QString id = QString::fromStdString(s.id);
        curIds.insert(id);
        curIdList << id;
        curLabels[id] = QString::fromStdString(s.name);
    }
    for (const SiteConfig& s : cfg.sites) {
        const QString id = QString::fromStdString(s.id);
        if (!prevIds.contains(id) && !st.sourceIds.isEmpty())   // nouveau (hors 1re synchro)
            res.addedLabels << QString::fromStdString(s.name);
    }
    for (const QString& id : st.sourceIds) {
        if (!curIds.contains(id)) {
            const QString label = st.labelsById.value(id).toString(id);
            res.removedLabels << label;                          // retire
        }
    }

    // 3. Decision de push (generation + revision).
    QJsonObject probe = buildWebManifest(cfg, providerInstance(), st.generation, st.revision);
    const QString hash = sourcesHash(probe);
    CollectorClient::Reply remote = CollectorClient::manifestState(baseUrl, providerInstance());
    const bool serverInSync = remote.ok()
        && remote.json.value("generation").toString() == st.generation
        && static_cast<qint64>(remote.json.value("revision").toDouble()) == st.revision;
    const bool contentChanged = (hash != st.hash);

    if (contentChanged || !serverInSync) {
        if (contentChanged) st.revision += 1;
        QJsonObject manifest = buildWebManifest(cfg, providerInstance(), st.generation, st.revision);
        CollectorClient::Reply pr = CollectorClient::pushManifest(baseUrl, manifest);
        if (!pr.ok()) {
            res.error = QStringLiteral("POST /manifest -> HTTP %1").arg(pr.status);
            return res;
        }
        st.hash = hash;
        res.pushed = true;
    }

    // 4. Secrets : les nouveaux sites toujours, tous si demande explicitement.
    // AUTO-GUERISON : si le coffre du collecteur porte moins de secrets qu'on a de
    // sites a lui confier (redeploiement, coffre reinitialise), on repousse tout.
    // Sans cela, apres un reset du coffre, la synchro d'ouverture repousse le
    // manifeste mais jamais les secrets -> sources bloquees en auth_failed.
    int localSecrets = 0;
    for (const SiteConfig& s : cfg.sites)
        if (!buildSecret(s).isEmpty()) ++localSecrets;
    if (cfg.github.enabled && !cfg.github.token.empty())
        ++localSecrets;
    const int remoteRefs = res.metrics.value(QStringLiteral("credentials_refs")).toInt();
    const bool vaultShort = remoteRefs < localSecrets;

    const QSet<QString> added(res.addedLabels.begin(), res.addedLabels.end());
    for (const SiteConfig& s : cfg.sites) {
        const QJsonObject secret = buildSecret(s);
        if (secret.isEmpty()) continue;
        const bool isNew = added.contains(QString::fromStdString(s.name));
        if (!forceCredentials && !vaultShort && !isNew && !st.sourceIds.isEmpty())
            continue;
        const QString ref = QStringLiteral("sw-") + QString::fromStdString(s.id);
        if (CollectorClient::pushCredentials(baseUrl, ref, secret).status == 204)
            ++res.credentialsPushed;
    }

    // 5. Persister l'etat (ids + libelles courants).
    st.sourceIds = curIdList;
    st.labelsById = curLabels;
    saveState(configPath, st);

    // 6. Manifeste GitHub DISTINCT : autre instance fournisseur pour que la
    // reconciliation SFTP ne retire jamais les sources github-traffic, et
    // inversement. Le jeton n'entre jamais dans le manifeste.
    State gst = loadState(configPath + QStringLiteral(".github"));
    if (gst.generation.isEmpty()) {
        gst.generation = QUuid::createUuid().toString(QUuid::WithoutBraces);
        gst.revision = 0;
    }
    const QString ghInstance = githubProviderInstance();
    QJsonObject ghProbe = buildGithubManifest(cfg, ghInstance, gst.generation, gst.revision);
    const QString ghHash = sourcesHash(ghProbe);
    CollectorClient::Reply ghRemote = CollectorClient::manifestState(baseUrl, ghInstance);
    const bool ghInSync = ghRemote.ok()
        && ghRemote.json.value("generation").toString() == gst.generation
        && static_cast<qint64>(ghRemote.json.value("revision").toDouble()) == gst.revision;
    const bool ghChanged = (ghHash != gst.hash);
    if (ghChanged || !ghInSync) {
        if (ghChanged) gst.revision += 1;
        QJsonObject ghManifest = buildGithubManifest(cfg, ghInstance, gst.generation, gst.revision);
        CollectorClient::Reply pr = CollectorClient::pushManifest(baseUrl, ghManifest);
        if (!pr.ok()) {
            res.error = QStringLiteral("POST /manifest (GitHub) -> HTTP %1").arg(pr.status);
            return res;
        }
        gst.hash = ghHash;
        res.pushed = true;
    }
    if (cfg.github.enabled && !cfg.github.token.empty()
        && (forceCredentials || vaultShort || ghChanged || gst.sourceIds.isEmpty())) {
        const QJsonObject secret{{QStringLiteral("token"),
                                  QString::fromStdString(cfg.github.token)}};
        if (CollectorClient::pushCredentials(baseUrl, githubCredentialsRef(cfg), secret).status == 204)
            ++res.credentialsPushed;
    }
    QStringList ghIds;
    QJsonObject ghLabels;
    if (cfg.github.enabled) {
        for (const GitHubRepoConfig& r : cfg.github.repositories) {
            if (!r.enabled)
                continue;
            const QString id = githubSourceId(cfg, r);
            ghIds << id;
            ghLabels[id] = QString::fromStdString(cfg.github.owner + "/" + r.name);
        }
    }
    gst.sourceIds = ghIds;
    gst.labelsById = ghLabels;
    saveState(configPath + QStringLiteral(".github"), gst);

    res.ok = true;
    res.revision = st.revision;
    res.sources = static_cast<int>(cfg.sites.size() + ghIds.size());
    return res;
}

FillResult fillCache(const QString& baseUrl, const SiteConfig& site,
                     const std::string& cacheRoot) {
    FillResult r;
    CollectorClient::Reply objs = CollectorClient::getObjects(baseUrl, QString::fromStdString(site.id));
    if (!objs.ok()) {
        r.error = objs.error.isEmpty() ? QStringLiteral("GET /objects -> HTTP %1").arg(objs.status)
                                       : objs.error;
        return r;
    }

    const std::string dir = cacheRoot + "/" + site.name;
    std::error_code ec;
    fs::create_directories(dir, ec);

    for (const QJsonValue& v : objs.json.value(QStringLiteral("objects")).toArray()) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QStringLiteral("original_name")).toString();
        const qint64  size = static_cast<qint64>(o.value(QStringLiteral("size")).toDouble());
        const QString oid  = o.value(QStringLiteral("object_id")).toString();
        if (name.isEmpty() || oid.isEmpty())
            continue;

        const std::string local = dir + "/" + name.toStdString();
        // Deja present et de meme taille : rien a faire (comme le telechargement
        // direct). Une taille differente = donnees plus recentes -> on recopie.
        if (fs::exists(local, ec) && static_cast<qint64>(fs::file_size(local, ec)) == size) {
            ++r.skipped;
            continue;
        }

        bool ok = false; QString derr;
        const QByteArray bytes = CollectorClient::download(baseUrl, oid, ok, derr);
        if (!ok) { r.error = derr; continue; }
        std::ofstream out(local, std::ios::binary | std::ios::trunc);
        if (!out) { r.error = QStringLiteral("ecriture cache impossible : %1").arg(name); continue; }
        out.write(bytes.constData(), bytes.size());
        ++r.downloaded;
    }

    r.ok = r.error.isEmpty();
    return r;
}

} // namespace collectorsync
