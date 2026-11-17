#!/usr/bin/env bash
# Copie a cote de l'executable toutes les DLL non-Qt de /ucrt64/bin (ou /mingw64/bin) dont
# depend l'application (libssh2, freetype, harfbuzz, runtime gcc, zlib,
# openssl...). windeployqt ne s'occupe que des DLL Qt ; ce script complete
# pour rendre l'exe lançable hors du shell MSYS2 (double-clic).
#
# Usage : deploy-mingw.sh <dossier_de_l_executable> [<bin_de_la_toolchain>]
#
# Le 2e argument (le dossier du compilateur, ex. /c/msys64/ucrt64/bin) sert a ALIGNER les DLL
# deja presentes : une DLL du dossier qui existe aussi dans ce bin et en differe est remplacee.
# Sans cela, un dossier compile avec un environnement MSYS2 puis un autre (mingw64 / ucrt64)
# garde la libstdc++ du premier a cote d'un exe lie au second (« point d'entree introuvable »).
set -e
# Appelé depuis CMake/PowerShell, ce script ne reçoit pas forcément le PATH du
# shell MSYS2. Les utilitaires Unix et la toolchain sont donc rendus explicites.
export PATH="/ucrt64/bin:/mingw64/bin:/usr/bin:$PATH"
cd "$1" 2>/dev/null || exit 0
TOOLCHAIN_BIN="${2:-}"
if [ -n "$TOOLCHAIN_BIN" ] && [ -d "$TOOLCHAIN_BIN" ]; then
  for f in *.dll; do
    [ -f "$f" ] && [ -f "$TOOLCHAIN_BIN/$f" ] && ! cmp -s "$TOOLCHAIN_BIN/$f" "$f" \
      && cp "$TOOLCHAIN_BIN/$f" "$f"
  done
fi
for f in *.exe *.dll */*.dll; do
  [ -f "$f" ] && ldd "$f" 2>/dev/null
done | grep -iE '/(ucrt64|mingw64)/bin/' | awk '{print $3}' | sort -u | while read -r dll; do
  # Collecte des DLL manquantes (l'alignement des DLL deja la est fait plus haut).
  cp "$dll" . 2>/dev/null || true
done
