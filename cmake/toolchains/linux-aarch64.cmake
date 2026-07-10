# Toolchain CMake : cross-compilation vers Linux ARM64 (aarch64).
#
# Utilisée par le preset "linux-arm64-cross" (voir CMakePresets.json).
#
# Prérequis côté machine hôte :
#   - une toolchain croisée "aarch64-linux-gnu-" (gcc/g++) ;
#   - un SYSROOT Linux ARM64 contenant Qt6, zlib, libssh2 et nlohmann-json
#     (headers + bibliothèques pour l'architecture cible).
#
# Variables d'environnement reconnues (optionnelles) :
#   SITEWATCH_CROSS_PREFIX : préfixe des outils (défaut : "aarch64-linux-gnu-")
#   SITEWATCH_SYSROOT      : chemin du sysroot cible (racine de recherche)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(DEFINED ENV{SITEWATCH_CROSS_PREFIX})
    set(_sw_prefix "$ENV{SITEWATCH_CROSS_PREFIX}")
else()
    set(_sw_prefix "aarch64-linux-gnu-")
endif()

set(CMAKE_C_COMPILER   "${_sw_prefix}gcc")
set(CMAKE_CXX_COMPILER "${_sw_prefix}g++")

# Sysroot cible : indispensable pour retrouver Qt6 et les autres dépendances
# compilées pour ARM64. Fourni par l'utilisateur via SITEWATCH_SYSROOT.
if(DEFINED ENV{SITEWATCH_SYSROOT})
    set(CMAKE_SYSROOT "$ENV{SITEWATCH_SYSROOT}")
    list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{SITEWATCH_SYSROOT}")
endif()

# Les exécutables (moc, rcc…) sont cherchés sur l'HÔTE ; les bibliothèques,
# en-têtes et paquets dans la CIBLE (sysroot).
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config doit chercher dans le sysroot (repli libssh2 de CMakeLists.txt).
if(DEFINED ENV{SITEWATCH_SYSROOT})
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{SITEWATCH_SYSROOT}")
endif()
