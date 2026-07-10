# Compiler SiteWatch depuis les sources

Retour à l'[index de la documentation](README.md).

Cette page donne les commandes rapides pour compiler SiteWatch sous Windows et
Linux. Pour un pas à pas destiné aux débutants, suivez les guides dédiés :

- Windows : [Compiler SiteWatch quand on débute](BUILD_FOR_BEGINNERS.md)
- Linux : [Installer et lancer SiteWatch sous Linux](INSTALL_LINUX.md)

> Rappel : aucun composant n'est à installer sur les serveurs analysés. SiteWatch
> fonctionne uniquement à partir des journaux Apache/LiteSpeed téléchargés en
> SFTP.

---

## Windows

La méthode officielle repose sur **MSYS2**, **MinGW** et **VS Code**.

L'installation des dépendances s'effectue avec une seule commande :

```bash
pacman -S --needed mingw-w64-x86_64-{gcc,cmake,ninja,qt6-base,qt6-charts,libssh2,zlib,nlohmann-json}
```

Compilation :

```bash
cmake --preset mingw
cmake --build --preset mingw
```

Exécution :

```bash
./build-mingw/SiteWatch.exe
```

Le guide détaillé pour débuter est disponible dans
[BUILD_FOR_BEGINNERS.md](BUILD_FOR_BEGINNERS.md).

---

## Linux

La compilation native Linux est fonctionnelle. Elle s'appuie sur :

- GCC ou Clang
- CMake + Ninja
- Qt6 (base + charts)
- libssh2
- zlib
- nlohmann-json

Installation des dépendances (exemple Debian/Ubuntu) puis compilation :

```bash
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-charts-dev qt6-base-dev-tools \
  libssh2-1-dev zlib1g-dev nlohmann-json3-dev

cmake --preset linux
cmake --build --preset linux
./build/SiteWatch
```

Pour intégrer SiteWatch au bureau (icône + raccourci) ou produire une AppImage
autonome, le guide pas à pas couvre chaque distribution :
[INSTALL_LINUX.md](INSTALL_LINUX.md).

---

## Presets disponibles

`CMakePresets.json` fournit plusieurs cibles (lister avec
`cmake --list-presets`) :

| Preset | Cible | Où l'exécuter |
|---|---|---|
| `linux` | Linux x86_64 (natif) | machine Linux x86_64 **ou WSL2** sous Windows |
| `linux-arm64` | Linux ARM64 (natif) | machine Linux aarch64 (Raspberry Pi, serveur ARM…) |
| `linux-arm64-cross` | Linux ARM64 (croisé) | machine x86_64 avec toolchain croisée + sysroot (automatisation future) |
| `mingw` | Windows x86_64 | Windows (MSYS2/MinGW) |

> Windows se compile uniquement en **MinGW** (voie officielle et simple depuis
> VS Code). Pour produire un binaire **Linux depuis Windows**, on n'utilise plus
> de cross-compilation : on ouvre un vrai Linux via **WSL2** et on y lance le
> preset `linux` (voir plus bas).

---

## Linux depuis Windows : WSL2 (recommandé)

Depuis Windows, la façon la plus simple de produire un binaire **Linux x86_64**
est **WSL2** : Windows dispose alors d'un véritable environnement Linux, et la
compilation devient **native** (aucun SDK Qt spécifique, aucun sysroot, aucune
copie de bibliothèques Qt à maintenir).

1. Installer WSL2 + Ubuntu : `wsl --install` (dans PowerShell).
2. Dans VS Code, installer l'extension **Remote - WSL** et ouvrir le projet dans
   Ubuntu (« Reopen in WSL »).
3. Installer les dépendances et compiler avec le preset `linux` (section Linux
   ci-dessus) :

```bash
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-charts-dev qt6-base-dev-tools \
  libssh2-1-dev zlib1g-dev nlohmann-json3-dev
cmake --preset linux
cmake --build --preset linux
```

---

## Linux ARM64 (natif)

Sur une machine **ARM64** (Raspberry Pi OS 64 bits, Ubuntu arm64, serveur ARM…),
la compilation est identique à x86_64 : installez les mêmes paquets Qt6/libssh2,
puis utilisez le preset dédié (dossier de build séparé `build-arm64`) :

```bash
cmake --preset linux-arm64
cmake --build --preset linux-arm64
./build-arm64/SiteWatch
```

La compilation native sur le Raspberry utilise exactement les bibliothèques de la
machine : aucun sysroot ni copie de Qt à gérer. C'est aujourd'hui la voie ARM
recommandée. **Compilation et exécution vérifiées sur Raspberry Pi 4** (Raspberry
Pi OS 64 bits).

---

## Cross-compilation ARM64 (optionnel, automatisation future)

Le preset `linux-arm64-cross` est **conservé comme base pour une future
automatisation** (GitHub Actions, serveur CI…) : il permettra de produire des
binaires Raspberry directement depuis une machine x86_64, sans y compiler
nativement. Il n'est **pas nécessaire** au quotidien — la compilation native sur
le Raspberry reste la voie recommandée aujourd'hui.

La cross-compilation d'une application **Qt** exige un **sysroot** de la cible
contenant Qt6, zlib, libssh2 et nlohmann-json (en-têtes + bibliothèques ARM64),
en plus d'une **toolchain croisée**. Le fichier de toolchain est dans
[`cmake/toolchains/`](../../cmake/toolchains).

Deux variables d'environnement (optionnelles) pilotent la toolchain :

- `SITEWATCH_SYSROOT` : chemin du sysroot de la cible ;
- `SITEWATCH_CROSS_PREFIX` : préfixe des outils (par défaut `aarch64-linux-gnu-`).

```bash
# toolchain croisée (Debian/Ubuntu)
sudo apt install crossbuild-essential-arm64
# … + un sysroot ARM64 fournissant Qt6/libssh2/zlib/nlohmann-json
export SITEWATCH_SYSROOT=/chemin/vers/sysroot-arm64

cmake --preset linux-arm64-cross
cmake --build --preset linux-arm64-cross
```
