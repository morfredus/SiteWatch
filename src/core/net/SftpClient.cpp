/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "core/net/SftpClient.h"

// --- Couche socket portable (Windows Winsock / POSIX) -----------------------
#ifdef _WIN32
  // L'ordre compte : winsock2 AVANT tout en-tete tirant windows.h.
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  static constexpr socket_t kInvalidSocket = INVALID_SOCKET;
  static inline void closeSocket(socket_t s) { closesocket(s); }
  static inline void setBlocking(socket_t s, bool on) {
      u_long mode = on ? 0 : 1;
      ioctlsocket(s, FIONBIO, &mode);
  }
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <cerrno>
  using socket_t = int;
  static constexpr socket_t kInvalidSocket = -1;
  static inline void closeSocket(socket_t s) { ::close(s); }
  static inline void setBlocking(socket_t s, bool on) {
      const int flags = fcntl(s, F_GETFL, 0);
      fcntl(s, F_SETFL, on ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
  }
#endif

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <fstream>
#include <cstring>

// Raccourcis de transtypage vers les types opaques stockes en void*.
#define SESSION (static_cast<LIBSSH2_SESSION*>(session_))
#define SFTP    (static_cast<LIBSSH2_SFTP*>(sftp_))

namespace {

// Delais reseau : sans eux, un serveur muet ou un pare-feu qui jette les paquets fige
// l'appelant pendant la valeur par defaut de l'OS (plusieurs minutes).
constexpr int  kConnectTimeoutSec = 15;      // etablissement TCP
constexpr long kSessionTimeoutMs  = 30000;   // handshake, auth, operations libssh2

// Connexion TCP avec delai : mode non bloquant + select, puis retour en mode bloquant
// (libssh2 est pilote en bloquant, borne par libssh2_session_set_timeout).
bool connectWithTimeout(socket_t s, const sockaddr* addr, socklen_t len) {
    setBlocking(s, false);
    if (::connect(s, addr, len) != 0) {
#ifdef _WIN32
        const bool inProgress = WSAGetLastError() == WSAEWOULDBLOCK;
#else
        const bool inProgress = errno == EINPROGRESS;
#endif
        if (!inProgress) return false;
        fd_set wfds, efds;
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        FD_SET(s, &wfds);
        FD_SET(s, &efds);
        timeval tv{kConnectTimeoutSec, 0};
        // Le 1er argument de select est ignore sous Windows, requis ailleurs.
        if (select(static_cast<int>(s) + 1, nullptr, &wfds, &efds, &tv) <= 0) return false;
        int soErr = 0;
        socklen_t sl = sizeof(soErr);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soErr), &sl) != 0
            || soErr != 0) return false;
    }
    setBlocking(s, true);
    return true;
}

// Empreinte SHA-256 de la cle d'hote en hexadecimal minuscule, prefixee "sha256:".
std::string hostKeyFingerprint(LIBSSH2_SESSION* session) {
    const char* raw = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!raw) return {};
    static const char* hex = "0123456789abcdef";
    std::string out = "sha256:";
    for (int i = 0; i < 32; ++i) {
        const unsigned char b = static_cast<unsigned char>(raw[i]);
        out += hex[b >> 4];
        out += hex[b & 0x0F];
    }
    return out;
}

} // namespace

SftpClient::SftpClient() {
    libssh2_init(0);
}

SftpClient::~SftpClient() {
    disconnect();
    libssh2_exit();
}

bool SftpClient::connect(const SiteConfig& site, std::string& error) {
    // --- Initialisation Winsock (Windows uniquement) ---
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error = "Initialisation réseau (WSAStartup) impossible.";
        return false;
    }
    wsaInit_ = true;
#endif

    // --- Résolution DNS + connexion TCP (port 22) ---
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    if (getaddrinfo(site.host.c_str(), "22", &hints, &res) != 0 || !res) {
        error = "Résolution DNS échouée pour " + site.host;
        return false;
    }

    // Essaie chaque adresse resolue (IPv6 puis IPv4, par exemple) : une famille injoignable
    // ne doit pas condamner la connexion si l'autre repond.
    socket_t s = kInvalidSocket;
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == kInvalidSocket) continue;
        if (connectWithTimeout(s, ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen))) break;
        closeSocket(s);
        s = kInvalidSocket;
    }
    freeaddrinfo(res);
    if (s == kInvalidSocket) {
        error = "Connexion TCP impossible vers " + site.host + ":22 (délai de "
              + std::to_string(kConnectTimeoutSec) + " s par adresse).";
        return false;
    }
    sock_ = static_cast<uintptr_t>(s);

    // --- Handshake SSH ---
    session_ = libssh2_session_init();
    if (!session_) { error = "Initialisation de la session SSH impossible."; return false; }
    libssh2_session_set_blocking(SESSION, 1);
    libssh2_session_set_timeout(SESSION, kSessionTimeoutMs);
    if (libssh2_session_handshake(SESSION, s) != 0) {
        error = "Handshake SSH échoué (le serveur autorise-t-il SSH ?).";
        return false;
    }

    // --- Verification de la cle d'hote (TOFU) AVANT toute authentification ---
    // Le mot de passe ne doit jamais partir vers un serveur dont l'identite n'est pas
    // reconnue. 1re connexion : l'empreinte est memorisee par l'appelant (observedHostKey).
    // Ensuite : toute difference est refusee ; l'utilisateur doit l'oublier explicitement
    // (reglages du site) pour accepter une nouvelle cle.
    observedHostKey_.clear();
    hostKeyMismatch_ = false;
    const std::string fp = hostKeyFingerprint(SESSION);
    if (fp.empty()) {
        error = "Empreinte de la clé d'hôte SSH illisible : connexion refusée.";
        return false;
    }
    if (!site.hostKeySha256.empty() && site.hostKeySha256 != fp) {
        hostKeyMismatch_ = true;
        error = "La clé d'hôte SSH de " + site.host + " a CHANGÉ (mémorisée : "
              + site.hostKeySha256 + ", reçue : " + fp + "). Connexion refusée par sécurité. "
              "Si le changement est légitime (réinstallation du serveur), oubliez l'empreinte "
              "dans les réglages du site puis reconnectez-vous.";
        return false;
    }
    observedHostKey_ = fp;

    // --- Authentification (clé SSH prioritaire, repli sur le mot de passe) ---
    int  rc = -1;
    bool attempted = false;

    // 1. Clé SSH si fournie. La clé publique .pub est utilisée si présente,
    //    sinon libssh2 la déduit de la clé privée (backend OpenSSL).
    if (!site.keyFile.empty()) {
        attempted = true;
        const std::string pub = site.keyFile + ".pub";
        const bool hasPub = std::ifstream(pub).good();
        rc = libssh2_userauth_publickey_fromfile(
            SESSION, site.user.c_str(),
            hasPub ? pub.c_str() : nullptr,
            site.keyFile.c_str(), nullptr);
    }

    // 2. Repli automatique sur le mot de passe (clé absente ou refusée).
    if (rc != 0 && !site.password.empty()) {
        attempted = true;
        rc = libssh2_userauth_password(SESSION, site.user.c_str(), site.password.c_str());
    }

    if (!attempted) {
        error = "Aucune authentification : renseignez une clé SSH ou un mot de passe.";
        return false;
    }
    if (rc != 0) {
        error = "Authentification refusée (clé et/ou mot de passe incorrects).";
        return false;
    }

    // --- Ouverture du canal SFTP ---
    sftp_ = libssh2_sftp_init(SESSION);
    if (!sftp_) { error = "Ouverture du canal SFTP impossible."; return false; }
    return true;
}

std::vector<SftpClient::RemoteFile> SftpClient::listLogs(const std::string& dir,
                                                         std::string& error) {
    std::vector<RemoteFile> out;
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(SFTP, dir.c_str());
    if (!handle) {
        error = "Impossible d'ouvrir le dossier distant : " + dir;
        return out;
    }

    char name[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc;
    while ((rc = libssh2_sftp_readdir(handle, name, sizeof(name), &attrs)) > 0) {
        std::string fn(name, static_cast<size_t>(rc));
        if (fn.size() > 3 && fn.compare(fn.size() - 3, 3, ".gz") == 0) {
            RemoteFile rf;
            rf.name = fn;
            rf.size = (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attrs.filesize : 0;
            out.push_back(rf);
        }
    }
    libssh2_sftp_closedir(handle);
    return out;
}

bool SftpClient::download(const std::string& remotePath, const std::string& localPath,
                          std::string& error, uint64_t expectedSize,
                          const ProgressFn& progress) {
    LIBSSH2_SFTP_HANDLE* handle =
        libssh2_sftp_open(SFTP, remotePath.c_str(), LIBSSH2_FXF_READ, 0);
    if (!handle) {
        error = "Ouverture du fichier distant impossible : " + remotePath;
        return false;
    }

    std::ofstream out(localPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        libssh2_sftp_close(handle);
        error = "Écriture locale impossible : " + localPath;
        return false;
    }

    char buffer[32768];
    ssize_t n;
    uint64_t received = 0;
    while ((n = libssh2_sftp_read(handle, buffer, sizeof(buffer))) > 0) {
        out.write(buffer, n);
        received += static_cast<uint64_t>(n);
        if (progress) progress(received, expectedSize);
    }
    libssh2_sftp_close(handle);
    out.close();

    if (n < 0) {
        error = "Erreur pendant la lecture SFTP de " + remotePath;
        return false;
    }
    return true;
}

void SftpClient::disconnect() {
    if (sftp_)    { libssh2_sftp_shutdown(SFTP); sftp_ = nullptr; }
    if (session_) {
        libssh2_session_disconnect(SESSION, "SiteWatch: fin de session");
        libssh2_session_free(SESSION);
        session_ = nullptr;
    }
    if (sock_ != ~static_cast<uintptr_t>(0)) {
        closeSocket(static_cast<socket_t>(sock_));
        sock_ = ~static_cast<uintptr_t>(0);
    }
#ifdef _WIN32
    if (wsaInit_) { WSACleanup(); wsaInit_ = false; }
#endif
}
