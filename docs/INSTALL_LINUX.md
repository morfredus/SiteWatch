# Installer et lancer SiteWatch sous Linux

Ce guide explique, pas à pas, comment utiliser SiteWatch sous Linux. Il
s'adresse aussi bien à une personne qui souhaite **simplement lancer le
programme** qu'à une personne qui préfère **le compiler elle-même**.

Deux usages sont possibles :

| Objectif | Méthode | Section |
|---|---|---|
| Utiliser SiteWatch **sans rien compiler** | Télécharger l'**AppImage** fournie dans les releases | [Partie A](#partie-a--utiliser-sitewatch-sans-compiler-appimage) |
| **Compiler** puis intégrer au bureau | Compilation + `install.sh` | [Partie B](#partie-b--compiler-puis-installer) |
| **Produire** l'AppImage pour une release | `package-appimage.sh` | [Partie C](#partie-c--produire-lappimage-pour-une-release) |

En cas de blocage, voir la section [Dépannage](#dépannage).

---

## Partie A — Utiliser SiteWatch sans compiler (AppImage)

C'est la méthode la plus simple. Une **AppImage** est un fichier unique qui
contient l'application **et toutes ses dépendances** (dont Qt). Rien à installer,
rien à configurer : on télécharge, on autorise l'exécution, on lance.

### A.1. Télécharger le fichier

1. Ouvrir la page **Releases** du projet sur GitHub.
2. Dans la dernière version, télécharger le fichier nommé :

   ```text
   SiteWatch-1.3.0-x86_64.AppImage
   ```

   (`x86_64` correspond aux ordinateurs 64 bits habituels, de type Intel ou AMD.)

### A.2. Autoriser l'exécution

Par sécurité, Linux ne rend pas un fichier téléchargé exécutable
automatiquement. Il faut donc l'autoriser une fois.

**Avec la souris** (fonctionne sur la plupart des bureaux, GNOME, KDE, XFCE) :

1. clic droit sur le fichier → **Propriétés** ;
2. onglet **Permissions** ;
3. cocher **Autoriser l'exécution du fichier comme un programme**.

**Avec le terminal** (équivalent, dans le dossier de téléchargement) :

```bash
chmod +x SiteWatch-1.3.0-x86_64.AppImage
```

`chmod +x` signifie « rendre ce fichier exécutable ».

### A.3. Lancer l'application

- **Avec la souris** : double-cliquer sur le fichier.
- **Avec le terminal** :

  ```bash
  ./SiteWatch-1.3.0-x86_64.AppImage
  ```

  Le `./` au début indique « le fichier situé dans le dossier courant ».

L'application s'ouvre directement. Il est possible de déplacer l'AppImage où
l'on veut (par exemple dans un dossier `~/Applications`) : elle reste
autonome.

### A.4. (Optionnel) Ajouter une icône au menu

Une AppImage fonctionne sans installation, mais n'apparaît pas d'office dans le
menu des applications. Pour l'y ajouter automatiquement (icône, raccourci), un
outil dédié est recommandé :

- **Gear Lever** (disponible sur Flathub) ;
- ou **AppImageLauncher**.

Ces outils proposent d'« intégrer » l'AppImage au premier lancement et créent
l'icône pour vous.

---

## Partie B — Compiler puis installer

À réserver aux personnes qui veulent compiler SiteWatch depuis les sources. À la
différence de l'AppImage, cette méthode s'appuie sur le **Qt du système** : les
dépendances doivent être présentes sur la machine.

### B.1. Installer les dépendances

Choisir la commande correspondant à la distribution.

**Debian / Ubuntu / Linux Mint :**

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-charts-dev qt6-base-dev-tools \
  libssh2-1-dev zlib1g-dev nlohmann-json3-dev
```

**Fedora :**

```bash
sudo dnf install gcc-c++ cmake ninja-build \
  qt6-qtbase-devel qt6-qtcharts-devel \
  libssh2-devel zlib-devel nlohmann-json-devel
```

**Arch Linux / Manjaro :**

```bash
sudo pacman -S --needed base-devel cmake ninja \
  qt6-base qt6-charts libssh2 zlib nlohmann-json
```

### B.2. Compiler

Depuis le dossier du projet :

```bash
cmake --preset linux
cmake --build --preset linux
```

- `cmake --preset linux` prépare la compilation (dossier `build/`).
- `cmake --build --preset linux` compile réellement le programme.

L'exécutable est alors disponible ici :

```text
build/SiteWatch
```

Pour le lancer immédiatement, sans rien installer :

```bash
./build/SiteWatch
```

### B.3. Installer l'icône et le raccourci

Le script `scripts/linux/install.sh` copie le programme dans les dossiers
standards de Linux et **crée l'icône de lancement** (raccourci d'application
visible dans le menu).

**Installation pour l'utilisateur courant** (recommandée, sans `sudo`) :

```bash
scripts/linux/install.sh
```

Le programme est installé dans `~/.local` :

- binaire : `~/.local/bin/sitewatch` ;
- raccourci : `~/.local/share/applications/sitewatch.desktop` ;
- icônes : `~/.local/share/icons/hicolor/…`.

SiteWatch apparaît alors dans le menu des applications. Il est aussi possible de
le lancer en tapant `sitewatch` dans un terminal.

**Installation pour tous les utilisateurs** (dans `/usr/local`, nécessite les
droits administrateur) :

```bash
sudo scripts/linux/install.sh --system
```

**Options utiles :**

| Option | Effet |
|---|---|
| *(aucune)* | Installe pour l'utilisateur courant (`~/.local`) |
| `--system` | Installe pour tous les utilisateurs (`/usr/local`, avec `sudo`) |
| `--binary <chemin>` | Utilise un binaire précis au lieu de celui de `build/` |
| `--uninstall` | Désinstalle (retire binaire, icône et raccourci) |
| `-h`, `--help` | Affiche l'aide |

**Désinstaller :**

```bash
scripts/linux/install.sh --uninstall
```

> Astuce : si les icônes sont générées à plusieurs tailles, installer d'abord
> **ImageMagick** (`sudo apt install imagemagick`, ou l'équivalent). Sans lui, le
> script se contente d'une icône unique — l'application fonctionne quand même.

---

## Partie C — Produire l'AppImage pour une release

Cette partie concerne la personne qui **prépare les releases** (par exemple le
mainteneur). Elle génère le fichier `.AppImage` distribué en [Partie A](#partie-a--utiliser-sitewatch-sans-compiler-appimage).

Prérequis : SiteWatch déjà compilé (voir [B.1](#b1-installer-les-dépendances) et
[B.2](#b2-compiler)) et une connexion internet au **premier** lancement (le
script télécharge une fois pour toutes ses outils dans `.cache/`).

```bash
cmake --preset linux
cmake --build --preset linux
scripts/linux/package-appimage.sh
```

Le résultat est produit dans :

```text
dist/SiteWatch-1.3.0-x86_64.AppImage
```

Ce fichier est celui à joindre à la release GitHub. Le numéro de version est lu
automatiquement dans le fichier `VERSION`.

---

## Où SiteWatch range ses données

Quelle que soit la méthode, la configuration et le cache sont stockés dans les
emplacements standards de l'utilisateur (jamais dans le dossier du programme) :

| Élément | Emplacement Linux |
|---|---|
| Configuration (`config.json`) | `~/.config/SiteWatch/` |
| Cache des logs (par défaut) | `~/.local/share/SiteWatch/cache/` |
| Préférence de thème | via les réglages utilisateur (QSettings) |

Le mot de passe et le jeton d'API ne sont enregistrés que dans ce fichier local.

---

## Dépannage

### L'AppImage ne se lance pas : « … cannot mount … » ou erreur FUSE

Certaines distributions récentes n'installent plus **FUSE 2**, requis par les
AppImages. Deux solutions :

- installer la bibliothèque, par exemple sur Debian/Ubuntu :

  ```bash
  sudo apt install libfuse2
  ```

- ou lancer l'AppImage en mode « extraction », qui n'a pas besoin de FUSE :

  ```bash
  ./SiteWatch-1.3.0-x86_64.AppImage --appimage-extract-and-run
  ```

### « Permission refusée » au lancement de l'AppImage

Le fichier n'a pas encore été rendu exécutable. Reprendre l'étape
[A.2](#a2-autoriser-lexécution).

### La commande `sitewatch` n'est pas trouvée après `install.sh`

En installation utilisateur, le programme est dans `~/.local/bin`, qui n'est pas
toujours présent dans le `PATH`. Le raccourci du **menu des applications**
fonctionne malgré tout. Pour utiliser la commande dans un terminal, ajouter ce
dossier au `PATH` (par exemple dans `~/.bashrc`) :

```bash
export PATH="$HOME/.local/bin:$PATH"
```

Puis rouvrir le terminal.

### L'icône n'apparaît pas tout de suite dans le menu

Les environnements de bureau mettent leur cache à jour périodiquement. Se
déconnecter puis se reconnecter suffit généralement à faire apparaître l'icône.

### Erreur CMake : un paquet Qt est introuvable

Une dépendance manque. Revenir à l'étape [B.1](#b1-installer-les-dépendances) et
vérifier que **tous** les paquets de la distribution sont bien installés (en
particulier `qt6-charts-dev` / `qt6-qtcharts-devel` / `qt6-charts`).

### Erreur de configuration persistante après un changement (cache CMake obsolète)

CMake mémorise le résultat de sa configuration dans le dossier `build/`. Après un
changement d'environnement — dépendance installée entre-temps, mise à jour de Qt,
bascule de branche — ce cache peut rester **incohérent** et faire échouer la
configuration ou la compilation alors que tout est pourtant en place. Symptôme
typique : une erreur qui persiste d'un essai à l'autre, par exemple
`Could not find XKB (missing: XKB_LIBRARY XKB_INCLUDE_DIR)`.

Solution : **nettoyer le cache CMake puis reconfigurer**.

En ligne de commande :

```bash
rm -rf build
cmake --preset linux && cmake --build --preset linux
```

Sous **VS Code** (extension CMake Tools) : `Ctrl+Maj+P` →
**CMake: Delete Cache and Reconfigure**, puis relancer la compilation.

C'est le premier réflexe à avoir devant toute erreur de compilation
inhabituelle : dans la grande majorité des cas, un cache propre suffit.

---

## Et ensuite ?

SiteWatch démarre correctement ? Passer au [Guide utilisateur](../GUIDE.md) pour
configurer un premier site, synchroniser les journaux et lancer l'analyse.
