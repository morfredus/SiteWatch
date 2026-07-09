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
)

set(CORE_HEADERS
    src/config/Config.h
    src/core/io/GzReader.h
    src/core/parser/ApacheLogParser.h
    src/core/analytics/BotDetector.h
    src/core/analytics/StatsEngine.h
    src/core/cache/CacheManager.h
    src/core/net/SftpClient.h
    src/core/model/LogEntry.h
    src/core/model/Stats.h
)

# L'interface (ui/) est le SEUL endroit qui depend de Qt.
set(UI_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.cpp
    src/ui/DetailDialog.cpp
    src/ui/UrlReport.cpp
    src/ui/ComparisonDialog.cpp
    src/ui/CacheCleanupDialog.cpp
)

set(UI_HEADERS
    src/ui/MainWindow.h
    src/ui/SettingsDialog.h
    src/ui/DetailDialog.h
    src/ui/UrlReport.h
    src/ui/ComparisonDialog.h
    src/ui/CacheCleanupDialog.h
)

# Ressources : icone Qt (barre de titre) + .rc Windows (icone de l'exe).
set(APP_RESOURCES resources/app.qrc)
if(WIN32)
    list(APPEND APP_RESOURCES resources/app.rc)
endif()

add_executable(SiteWatch WIN32
    ${CORE_SOURCES}
    ${CORE_HEADERS}
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

# MinGW/MSYS2 : windeployqt ne copie QUE les DLL Qt. Ce script complete avec les
# DLL non-Qt de /mingw64/bin (libssh2, freetype, runtime gcc, zlib, openssl...)
# pour rendre l'exe lancable en dehors du shell MSYS2 (double-clic).
if(WIN32 AND NOT MSVC)
    add_custom_command(TARGET SiteWatch POST_BUILD
        COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/deploy-mingw.sh" "$<TARGET_FILE_DIR:SiteWatch>"
        COMMENT "Copie des DLL MinGW a cote de l'executable")
endif()
