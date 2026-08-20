cmake_minimum_required(VERSION 3.21)

set(SITEWATCH_VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SITEWATCH_VERSION_FILE}")
file(STRINGS "${SITEWATCH_VERSION_FILE}" SITEWATCH_VERSION_FROM_FILE LIMIT_COUNT 1)
string(STRIP "${SITEWATCH_VERSION_FROM_FILE}" SITEWATCH_VERSION_FROM_FILE)

project(SiteWatch VERSION "${SITEWATCH_VERSION_FROM_FILE}" LANGUAGES CXX)

# --- Standard C++ ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt : generation automatique (moc / uic / rcc) ------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Dependances ----------------------------------------------------------
# Qt6 fournit l'interface, les graphiques (Charts) et le reseau.
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Network)
# zlib : lecture des fichiers de log compresses (.gz).
# Avec MinGW, CMake lancé depuis PowerShell ne déduit pas toujours le préfixe
# de la toolchain à partir du compilateur. Le retrouver évite de figer un chemin
# MSYS2 tout en laissant les autres plateformes à leur détection normale.
if(WIN32 AND MINGW AND NOT ZLIB_ROOT)
    get_filename_component(SITEWATCH_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(SITEWATCH_MINGW_ROOT "${SITEWATCH_MINGW_BIN}" DIRECTORY)
    if(EXISTS "${SITEWATCH_MINGW_ROOT}/include/zlib.h")
        set(ZLIB_ROOT "${SITEWATCH_MINGW_ROOT}")
    endif()
endif()
find_package(ZLIB REQUIRED)
# nlohmann-json : lecture du fichier de configuration.
find_package(nlohmann_json CONFIG REQUIRED)

# libssh2 : connexion SSH/SFTP. Le nom du package/cible varie selon la chaine
# (MSYS2/MinGW, paquets systeme) -> detection robuste, repli pkg-config.
find_package(libssh2 CONFIG QUIET)
if(NOT TARGET libssh2::libssh2 AND NOT TARGET Libssh2::libssh2)
    find_package(Libssh2 CONFIG QUIET)
endif()
if(TARGET libssh2::libssh2)
    set(LIBSSH2_TARGET libssh2::libssh2)
elseif(TARGET Libssh2::libssh2)
    set(LIBSSH2_TARGET Libssh2::libssh2)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED IMPORTED_TARGET libssh2)
    set(LIBSSH2_TARGET PkgConfig::LIBSSH2)
endif()

# --- Modules communs « morf » (copies vendorées dans third_party/) ---------
# Autonome : aucune dépendance à un dépôt externe, compilé dans l'exécutable.
# Voir third_party/morf/README.md. Resynchro : scripts/sync-morf.(ps1|sh).
add_subdirectory(third_party/morf/beacon)
add_subdirectory(third_party/morf/update)

# --- Sources --------------------------------------------------------------
# Le coeur (core/) ne depend QUE de la bibliotheque standard, zlib et json.
# Il ne connait NI Qt NI le reseau : il reste portable.
set(CORE_SOURCES
    src/config/Config.cpp
    src/core/io/GzReader.cpp
    src/core/parser/ApacheLogParser.cpp
    src/core/analytics/BotDetector.cpp
    src/core/analytics/StatsEngine.cpp
    src/core/cache/CacheManager.cpp
    src/core/net/SftpClient.cpp
    src/core/net/LogDiscovery.cpp
)

set(CORE_HEADERS
    src/config/Config.h
    src/core/io/GzReader.h
    src/core/parser/ApacheLogParser.h
    src/core/analytics/BotDetector.h
    src/core/analytics/StatsEngine.h
    src/core/cache/CacheManager.h
    src/core/net/SftpClient.h
    src/core/net/LogDiscovery.h
    src/core/model/LogEntry.h
    src/core/model/Stats.h
)

# L'interface (ui/) est le SEUL endroit qui depend de Qt.
set(UI_SOURCES
    src/main.cpp
    src/ui/Theme.cpp
    src/ui/Icons.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.cpp
    src/ui/DetailDialog.cpp
    src/ui/UrlReport.cpp
    src/ui/CacheCleanupDialog.cpp
    src/ui/CoherenceDialog.cpp
)

set(UI_HEADERS
    src/ui/Theme.h
    src/ui/Icons.h
    src/ui/MainWindow.h
    src/ui/SettingsDialog.h
    src/ui/DetailDialog.h
    src/ui/UrlReport.h
    src/ui/CacheCleanupDialog.h
    src/ui/CoherenceDialog.h
)

# Integration morfCollector (cote fournisseur). Qt Network ; appele par l'UI.
set(COLLECTOR_SOURCES
    src/collector/CollectorClient.cpp
    src/collector/CollectorSync.cpp
)
set(COLLECTOR_HEADERS
    src/collector/CollectorClient.h
    src/collector/CollectorSync.h
)

# Controle de coherence (chantier B1). Le MOTEUR est pur (aucune E/S, aucune
# dependance Qt) : teste seul. L'ADAPTATEUR construit les trois vues reelles
# (SFTP o2switch, GET /objects collecteur, scan du cache) : depend de Qt/libssh2.
set(COHERENCE_SOURCES
    src/coherence/CoherenceEngine.cpp
)
set(COHERENCE_HEADERS
    src/coherence/CoherenceEngine.h
    src/coherence/CoherenceCheck.h
)
set(COHERENCE_IO_SOURCES
    src/coherence/CoherenceCheck.cpp
)

# Ressources : icone Qt (barre de titre) + .rc Windows (icone de l'exe).
set(APP_RESOURCES resources/app.qrc)
if(WIN32)
    list(APPEND APP_RESOURCES resources/app.rc)
endif()

add_executable(SiteWatch WIN32
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${COLLECTOR_SOURCES}
    ${COLLECTOR_HEADERS}
    ${COHERENCE_SOURCES}
    ${COHERENCE_IO_SOURCES}
    ${COHERENCE_HEADERS}
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${APP_RESOURCES}
)

target_include_directories(SiteWatch PRIVATE src)

# Numero de version accessible depuis le code (ex. menu "A propos").
target_compile_definitions(SiteWatch PRIVATE SITEWATCH_VERSION="${PROJECT_VERSION}")

# Force l'encodage UTF-8 des sources (accents francais + emoji dans l'interface).
if(MSVC)
    target_compile_options(SiteWatch PRIVATE /utf-8)
endif()

target_link_libraries(SiteWatch PRIVATE
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
    ${LIBSSH2_TARGET}
    morfBeacon::morfBeacon
    morfUpdate::morfUpdate
    morfUpdate::Widgets
)

# Winsock, requis par libssh2 sur Windows.
if(WIN32)
    target_link_libraries(SiteWatch PRIVATE ws2_32)
endif()

# Copie automatique des DLL Qt a cote de l'exe apres compilation (Windows).
if(WIN32)
    find_program(WINDEPLOYQT_EXE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET SiteWatch POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}" --no-translations --compiler-runtime "$<TARGET_FILE:SiteWatch>"
            COMMENT "Deploiement des DLL Qt a cote de l'executable")
    endif()
endif()

# --- Outil headless : synchronisation vers morfCollector ------------------
# Cote FOURNISSEUR du contrat morfcollect/1 (decouverte morfBeacon, push du
# manifeste et des secrets, lecture des copies). Console (Qt Core + Network), sans
# GUI : sert a l'integration et a la verification. La GUI reutilisera
# CollectorClient.
add_executable(sitewatch-collector-sync
    src/collector/sync_main.cpp
    src/collector/CollectorClient.cpp
    src/collector/CollectorClient.h
    src/collector/CollectorSync.cpp
    src/collector/CollectorSync.h
    src/config/Config.cpp
    src/config/Config.h
)
target_include_directories(sitewatch-collector-sync PRIVATE src)
target_link_libraries(sitewatch-collector-sync PRIVATE
    Qt6::Core
    Qt6::Network
    nlohmann_json::nlohmann_json
)
if(MSVC)
    target_compile_options(sitewatch-collector-sync PRIVATE /utf-8)
endif()

# --- Auto-test du moteur de coherence (chantier B1) -----------------------
# Console PURE (aucune E/S, aucune dependance Qt) : verifie que chaque scenario
# des trois vues (source / collecteur / cache) produit l'etat attendu. Sert de
# banc de demonstration avant tout cablage d'interface.
add_executable(sitewatch-coherence-selftest
    src/coherence/coherence_selftest.cpp
    ${COHERENCE_SOURCES}
    ${COHERENCE_HEADERS}
)
target_include_directories(sitewatch-coherence-selftest PRIVATE src)
if(MSVC)
    target_compile_options(sitewatch-coherence-selftest PRIVATE /utf-8)
endif()

# --- CLI headless : contrôle de cohérence end-to-end ----------------------
# Construit les trois vues REELLES (SFTP o2switch, collecteur, cache) et affiche
# l'etat par fichier. Metadonnees seulement, aucun .gz telecharge. Sert de
# verification reelle avant l'UI ; la GUI reutilisera checkSite().
add_executable(sitewatch-coherence
    src/coherence/coherence_main.cpp
    ${COHERENCE_SOURCES}
    ${COHERENCE_IO_SOURCES}
    ${COHERENCE_HEADERS}
    src/collector/CollectorClient.cpp
    src/collector/CollectorClient.h
    src/core/net/SftpClient.cpp
    src/core/net/SftpClient.h
    src/core/net/LogDiscovery.cpp
    src/core/net/LogDiscovery.h
    src/config/Config.cpp
    src/config/Config.h
)
target_include_directories(sitewatch-coherence PRIVATE src)
target_link_libraries(sitewatch-coherence PRIVATE
    Qt6::Core
    Qt6::Network
    nlohmann_json::nlohmann_json
    ${LIBSSH2_TARGET}
)
if(WIN32)
    target_link_libraries(sitewatch-coherence PRIVATE ws2_32)
endif()
if(MSVC)
    target_compile_options(sitewatch-coherence PRIVATE /utf-8)
endif()

# MinGW/MSYS2 : windeployqt ne copie QUE les DLL Qt. Ce script complete avec les
# DLL non-Qt de /mingw64/bin (libssh2, freetype, runtime gcc, zlib, openssl...)
# pour rendre l'exe lancable en dehors du shell MSYS2 (double-clic).
if(WIN32 AND NOT MSVC)
    function(sitewatch_msys_path windows_path output)
        string(REGEX REPLACE "^([A-Za-z]):/" "/\\1/" converted "${windows_path}")
        string(SUBSTRING "${converted}" 1 1 drive)
        string(TOLOWER "${drive}" drive)
        string(SUBSTRING "${converted}" 2 -1 tail)
        set(${output} "/${drive}${tail}" PARENT_SCOPE)
    endfunction()
    sitewatch_msys_path("${CMAKE_SOURCE_DIR}/scripts/windows/deploy-mingw.sh"
                        SITEWATCH_DEPLOY_SCRIPT)
    sitewatch_msys_path("${CMAKE_CURRENT_BINARY_DIR}" SITEWATCH_TARGET_DIR)
    get_filename_component(SITEWATCH_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(SITEWATCH_MINGW_ROOT "${SITEWATCH_COMPILER_BIN}" DIRECTORY)
    get_filename_component(SITEWATCH_MSYS_ROOT "${SITEWATCH_MINGW_ROOT}" DIRECTORY)
    set(SITEWATCH_MSYS_BASH "${SITEWATCH_MSYS_ROOT}/usr/bin/bash.exe")
    if(EXISTS "${SITEWATCH_MSYS_BASH}")
        add_custom_command(TARGET SiteWatch POST_BUILD
            # PATH peut résoudre `bash` vers WSL, sans accès au workspace
            # Windows. On utilise donc le Bash MSYS2 lié au compilateur actif.
            COMMAND "${SITEWATCH_MSYS_BASH}" "${SITEWATCH_DEPLOY_SCRIPT}" "${SITEWATCH_TARGET_DIR}"
            COMMENT "Copie des DLL MinGW a cote de l'executable")
    else()
        message(WARNING "MSYS2 Bash not found; non-Qt MinGW DLL collection is skipped")
    endif()
endif()
