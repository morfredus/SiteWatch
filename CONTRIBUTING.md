# Contribuer à SiteWatch

Merci de l'intérêt que vous portez à SiteWatch ! Ce document explique comment
compiler le projet, signaler un bug, proposer une modification, et les
conventions de code à respecter.

---

## Compiler le projet

Le détail complet est dans le [README](README.md). En résumé, sous Windows la
voie la plus simple est **MSYS2 / MinGW** :

```bash
# dans le shell « MSYS2 MINGW64 »
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts \
  mingw-w64-x86_64-libssh2 mingw-w64-x86_64-zlib mingw-w64-x86_64-nlohmann-json

cd /c/chemin/vers/SiteWatch
cmake --preset mingw
cmake --build --preset mingw
```

Les autres voies (WSL2, Linux natif, MSVC + vcpkg) sont décrites dans le README.
Le code doit rester **portable** : il compile sous Windows (MinGW/MSVC) et Linux.

---

## Signaler un bug

Ouvrez une **issue** sur le dépôt GitHub du projet en incluant :

- la **version** de SiteWatch (menu *Aide → À propos*, ou le fichier `VERSION`) ;
- votre **système** (Windows 10/11, Linux + distribution) ;
- les **étapes pour reproduire** le problème ;
- le **comportement attendu** et le comportement observé ;
- si possible une **capture d'écran** et le message d'erreur exact.

⚠️ Ne joignez **jamais** votre `config.json`, mots de passe, clés SSH ou jetons
d'API — anonymisez toute donnée sensible.

---

## Proposer une modification (Pull Request)

1. **Forkez** le dépôt et créez une branche descriptive
   (`fix/erreur-parseur`, `feat/export-json`…).
2. **Compilez et testez** votre modification (au moins la voie MinGW).
3. Faites des **commits clairs** (une idée par commit, message à l'impératif :
   « Ajoute le filtre par code HTTP »).
4. Respectez l'**architecture** et les **conventions de code** ci-dessous.
5. Ouvrez la **Pull Request** vers la branche principale avec une description de
   ce que fait le changement et pourquoi.

Chaque nouveau fichier source doit porter l'en-tête de licence :

```cpp
/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */
```

En contribuant, vous acceptez que votre contribution soit distribuée sous la
licence du projet (**GNU GPL v3.0**).

---

## Conventions de code

**Architecture** — la séparation est stricte :

- `src/core/` : le **cœur** métier (parseur, statistiques, cache, réseau). Il ne
  doit **jamais** inclure Qt ni dépendre de l'interface — il reste portable.
- `src/config/` : lecture/écriture de la configuration.
- `src/ui/` : l'**interface** (Qt). Seul endroit autorisé à utiliser Qt.

**Langage & style**

- **C++17**, CMake ≥ 3.21.
- Indentation : **4 espaces**, pas de tabulations.
- Accolades façon K&R (accolade ouvrante sur la même ligne).
- Nommage : classes en `PascalCase`, méthodes et variables en `camelCase`,
  **membres suffixés par `_`** (ex. `siteSelector_`), constantes `kMaConstante`.
- **Commentaires en français**, concis et utiles (le « pourquoi », pas le « quoi »).
- Fichiers sources encodés en **UTF-8** ; les chaînes visibles sont en français.
- Préférer les types et conteneurs de la bibliothèque standard dans `core/`,
  et les types Qt dans `ui/`.

**Qualité**

- Une classe = une responsabilité ; chaque module doit pouvoir évoluer seul.
- Pas de secret en dur dans le code (mots de passe, jetons) : tout passe par
  `config.json` (ignoré par Git).
- Le projet doit continuer à compiler **sans avertissement** sur MinGW.

---

*Développé par morfredus.*
