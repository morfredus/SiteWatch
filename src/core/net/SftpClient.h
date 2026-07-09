/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include "config/Config.h"

// -----------------------------------------------------------------------------
// SftpClient : télécharge les logs distants via SSH/SFTP (libssh2).
//
// Le détail bas niveau (Winsock, libssh2) est entièrement encapsulé dans le
// .cpp : cet en-tête ne dépend que de la bibliothèque standard et de SiteConfig.
//
// Cycle de vie :
//     SftpClient c;
//     if (c.connect(site, err)) {
//         auto files = c.listLogs(site.remoteLogDir, err);
//         c.download(remotePath, localPath, err);
//         c.disconnect();
//     }
// -----------------------------------------------------------------------------
class SftpClient {
public:
    struct RemoteFile {
        std::string name;      // nom du fichier (ex. "Jul-2026.gz")
        uint64_t    size = 0;  // taille distante en octets
    };

    SftpClient();
    ~SftpClient();

    SftpClient(const SftpClient&) = delete;
    SftpClient& operator=(const SftpClient&) = delete;

    // Connexion TCP + handshake SSH + authentification (password ou clé) + SFTP.
    bool connect(const SiteConfig& site, std::string& error);

    // Liste les fichiers .gz présents dans le dossier distant.
    std::vector<RemoteFile> listLogs(const std::string& remoteDir, std::string& error);

    // Callback de progression : (octets reçus, taille totale attendue).
    using ProgressFn = std::function<void(uint64_t received, uint64_t total)>;

    // Télécharge un fichier distant vers un chemin local. 'expectedSize' et
    // 'progress' sont optionnels et servent à alimenter une barre de progression.
    bool download(const std::string& remotePath, const std::string& localPath,
                  std::string& error, uint64_t expectedSize = 0,
                  const ProgressFn& progress = {});

    void disconnect();

private:
    void*     session_ = nullptr;              // LIBSSH2_SESSION*
    void*     sftp_    = nullptr;              // LIBSSH2_SFTP*
    uintptr_t sock_    = ~static_cast<uintptr_t>(0); // SOCKET (INVALID_SOCKET)
    bool      wsaInit_ = false;
};
