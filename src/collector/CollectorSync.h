/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <string>
#include "config/Config.h"

// -----------------------------------------------------------------------------
// CollectorSync : orchestration cote FOURNISSEUR du contrat morfcollect/1,
// reutilisable par la GUI (MainWindow) ET l'outil console
// (sitewatch-collector-sync). Au-dessus de CollectorClient, il :
//   - localise morfCollector (URL explicite ou decouverte morfBeacon) ;
//   - construit le manifeste depuis la configuration SiteWatch ;
//   - gere generation + revision (etat persiste a cote de config.json) ;
//   - pousse manifeste et secrets ;
//   - detecte les sites AJOUTES et RETIRES depuis la derniere synchro ;
//   - remplit le cache local d'un site depuis les copies du collecteur.
//
// Un retrait N'EFFACE JAMAIS les fichiers conserves sur le collecteur (CONTRAT.md
// §1.7) : il est seulement signale a l'utilisateur.
// -----------------------------------------------------------------------------
namespace collectorsync {

struct SyncResult {
    bool        ok = false;         // operation sans erreur de transport
    QString     error;
    bool        pushed = false;     // manifeste envoye (false = rien a faire)
    int         sources = 0;
    qint64      revision = 0;
    QStringList addedLabels;        // sites nouveaux depuis la derniere synchro
    QStringList removedLabels;      // sites retires depuis la derniere synchro
    int         credentialsPushed = 0;
    QJsonObject metrics;            // metriques du collecteur (/status)
};

struct FillResult {
    bool    ok = false;
    QString error;
    int     downloaded = 0;         // originaux recopies dans le cache
    int     skipped = 0;            // deja presents et inchanges
};

// Localise le collecteur : `explicitUrl` s'il est fourni, sinon decouverte
// morfBeacon (capacite 'collection'). Renvoie l'URL de base, ou "" si absent.
QString locate(const QString& explicitUrl, int discoverTimeoutMs);

// Synchronise la configuration vers le collecteur deja localise.
// `forceCredentials` : remettre TOUS les secrets (sinon seulement ceux des
// nouveaux sites) -- utile pour un « Tout synchroniser » explicite.
SyncResult synchronize(const QString& baseUrl, const Config& cfg,
                       const QString& configPath, bool forceCredentials);

// Remplit le cache local d'un site depuis le collecteur : ne recopie que les
// originaux absents, ou presents mais de taille differente (donnees plus
// recentes). Meme disposition de cache que le telechargement direct.
FillResult fillCache(const QString& baseUrl, const SiteConfig& site,
                     const std::string& cacheRoot);

} // namespace collectorsync
