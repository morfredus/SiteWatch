/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

// -----------------------------------------------------------------------------
// CollectorClient : cote FOURNISSEUR du contrat morfcollect/1 (voir
// morfCollector/docs/fr/CONTRAT.md). Headless, sans dependance a l'interface :
// la GUI de SiteWatch l'appellera, mais il se teste seul (outil console).
//
// Il :
//   - decouvre morfCollector sur le LAN via morfBeacon (capacite "collection",
//     jamais par son nom) ;
//   - verifie la compatibilite du protocole (bloc `collect` de /status) ;
//   - compare la revision appliquee (GET /manifest/state) ;
//   - pousse un manifeste (POST /manifest) et les secrets (POST /credentials) ;
//   - lit les copies locales (GET /sources, /sources/{id}/objects).
//
// Requetes synchrones (QEventLoop) : simple a piloter depuis un outil ou un
// thread de travail de la GUI.
// -----------------------------------------------------------------------------
class CollectorClient {
public:
    struct Discovered {
        bool        found = false;
        QString     host;          // adresse pour joindre /status (IP source du heartbeat)
        quint16     statusPort = 0;
        QString     app;           // nom affiche (jamais utilise comme cle)
        QStringList capabilities;
        QString     baseUrl() const {
            return QStringLiteral("http://%1:%2").arg(host).arg(statusPort);
        }
    };

    struct Reply {
        int         status = 0;    // code HTTP (0 = pas de reponse / erreur reseau)
        QJsonObject json;          // corps decode en objet (si applicable)
        QString     error;         // message en cas d'echec transport
        bool ok() const { return status >= 200 && status < 300; }
    };

    // Ecoute le heartbeat morfBeacon et renvoie le premier service annoncant la
    // capacite `collection`. false si rien avant `timeoutMs`.
    static bool discover(int timeoutMs, Discovered& out, QString& error);

    // Ecoute pendant TOUTE la fenetre `timeoutMs` et renvoie TOUS les collecteurs
    // vus (capacite `collection`), dedupliques par hote. Necessaire quand plusieurs
    // morfCollector coexistent : SiteWatch pousse a chacun mais n'en lit qu'un.
    static QVector<Discovered> discoverAll(int timeoutMs);

    // Verifie que /status annonce un `collect.proto` compatible avec `wanted`.
    static bool checkCompatible(const QString& baseUrl, const QString& wantedProto,
                                QJsonObject& status, QString& error);

    // Etat applique pour un fournisseur (found=false si 404).
    static Reply manifestState(const QString& baseUrl, const QString& instance);

    static Reply pushManifest(const QString& baseUrl, const QJsonObject& manifest);
    static Reply pushCredentials(const QString& baseUrl, const QString& ref,
                                 const QJsonObject& secret);
    static Reply getSources(const QString& baseUrl);
    static Reply getObjects(const QString& baseUrl, const QString& sourceId);
    static Reply getStatus(const QString& baseUrl);
    static Reply collectNow(const QString& baseUrl, const QString& sourceId);

    // Administration (CONTRAT.md §6.3). Toutes exécutées par le collecteur.
    static Reply suspend(const QString& baseUrl, const QString& sourceId);
    static Reply resume(const QString& baseUrl, const QString& sourceId);
    static Reply deleteObject(const QString& baseUrl, const QString& objectId);
    static Reply deleteSourceObjects(const QString& baseUrl, const QString& sourceId);
    static Reply deleteSource(const QString& baseUrl, const QString& sourceId);

    // Recupere l'original conserve (GET /objects/{id}) : reponse BINAIRE.
    static QByteArray download(const QString& baseUrl, const QString& objectId,
                               bool& ok, QString& error);
};
