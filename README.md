# SiteWatch


**Analyseur de logs Apache/LiteSpeed** pour administrer et superviser plusieurs sites web 
hébergés sur **o2switch** (et compatible avec d'autres hébergeurs SSH).

Application légère dédiée à l'administration système et à la supervision de sites web.
Aucune base SQL, aucune dépendance WordPress : lecture directe des fichiers `.gz`.

Pour l'utilisation pas à pas, voir le **[Guide utilisateur](GUIDE.md)**.
Pour compiler facilement depuis VS Code, voir
**[Compiler SiteWatch quand on débute](docs/BUILD_FOR_BEGINNERS.md)**.

## Why SiteWatch?

Les outils classiques répondent à : **« Combien de visiteurs ? »**
SiteWatch répond à : **« Pourquoi cette URL reçoit-elle des 404 ? »**
**« Quel robot scanne mon site ? »**
**« Pourquoi Google demande cette ressource ? »**
**« Quelle extension provoque cette activité ? »**

## Fonctionnalités

- **Téléchargement SFTP** des logs (libssh2) : seuls les fichiers nouveaux ou
  modifiés sont récupérés, avec barre de progression.
- **Ouverture automatique du pare-feu** o2switch (API cPanel) avant connexion.
- **Filtrage par site** : chaque site ne traite que ses propres logs (préfixe
  déduit automatiquement du nom de domaine, sous-domaines exclus).
- **Filtre de période** (jour, 7/30 jours, mois, année, personnalisé) —
  recalcule tous les onglets.
- **Résumé permanent** : total, humains, robots, erreurs 404 / 403 / 500.
- **Tableau de santé** 🟢/🟠/🔴 avec indicateurs cliquables (500, attaques,
  404, activité Google, robots IA) qui renvoient vers l'onglet concerné.
- **Détection de robots** classés par catégorie (IA, moteurs, SEO, divers) avec
  pourcentages et **donut** de répartition.
- **Sécurité** : distinction entre activité WordPress légitime et vraies
  tentatives d'attaque (anti-faux-positifs).
- **Onglet URLs** avec catégories : toutes, attaques probables, fonctionnement
  WordPress, erreurs 404, requêtes système.
- **Top pages, référents, graphiques** d'évolution.
- **Recherche** par IP, URL, robot, date ou code HTTP.
- **Onglets interactifs** : double-clic sur n'importe quelle ligne (Sécurité,
  Activité WP, Top pages, Référents, URLs, Recherche) pour son détail (IP,
  User-Agents, URLs, référents, horaires, codes, évolution) ; copie / export CSV
  d'une ou plusieurs lignes depuis chaque onglet.
- **Comparaison de sites** côte à côte sur une même période.
- **Nettoyage du cache** : suppression des logs par site, en totalité ou par
  période (au mois).
- **Configuration graphique** complète (aucune édition manuelle de JSON).

## Hébergement

SiteWatch est **développé et optimisé pour l'hébergement
[o2switch](https://www.o2switch.fr)** :

- **Pare-feu SSH** : autorisation automatique de l'IP publique via l'API cPanel
  (`SshWhitelist`, port 2083) avant chaque connexion — nécessite un *jeton d'API cPanel*.
- **Nommage des logs** : o2switch découpe les logs **mensuellement** et nomme le
  fichier du domaine principal `<domaine sans points>.<compte>.odns.fr-Mois-Année.gz`.
  Le préfixe de filtrage est déduit automatiquement du « nom du site ».

### Utiliser un autre hébergeur

L'architecture reste ouverte. Pour un hébergeur non-o2switch :

- **Pas de pare-feu à ouvrir** : laisser le champ *Jeton d'API cPanel* **vide**
  (l'étape d'autorisation est ignorée, connexion SSH directe).
- **Nommage de logs différent** : renseigner le champ **« Filtre des logs (avancé) »**
  dans la configuration du site — un fichier est retenu si son nom **contient** ce
  motif (ex. `monsite.fr`), ce qui remplace la détection automatique o2switch.

Seule la connexion reste en **SFTP standard** (libssh2) — compatible avec la plupart
des hébergeurs proposant un accès SSH.

## Compilation

Le code est portable et se compile avec des **paquets déjà précompilés** — pas
besoin de tout construire soi-même.

Pour débuter ou compiler depuis VS Code, commencer par le guide
**[Compiler SiteWatch quand on débute](docs/BUILD_FOR_BEGINNERS.md)**.

| Voie | Pour qui | Chaîne |
|---|---|---|
| **A. MSYS2 / MinGW** (Windows) | Débutants, la plus simple | GCC + Qt via `pacman` |
| **B. WSL2 / Ubuntu** (Windows) | À l'aise avec Linux | apt + WSLg |
| **C. Linux natif** | Utilisateurs Linux | apt |

Dépendances communes (fournies par le gestionnaire de paquets) : **Qt 6** (Widgets,
Charts, Network), **zlib**, **nlohmann-json**, **libssh2**, plus **CMake** et **Ninja**.

> Pourquoi pas MSVC/vcpkg ? Le projet garde une voie Windows officielle :
> **MSYS2/MinGW**. Ce n'est pas une régression, mais une simplification :
> un seul environnement à documenter, tester et dépanner, avec toutes les
> dépendances installées par `pacman`.

### A — Windows avec MSYS2 / MinGW (recommandé)

1. Installer **[MSYS2](https://www.msys2.org)** (ou `winget install MSYS2.MSYS2`).
2. Ouvrir le raccourci **« MSYS2 MINGW64 »** (menu Démarrer), puis mettre à jour :
   ```bash
   pacman -Syu        # relancer le shell si demandé, puis relancer la commande
   ```
3. Installer la chaîne et les dépendances (précompilées) :
   ```bash
   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
     mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts \
     mingw-w64-x86_64-libssh2 mingw-w64-x86_64-zlib mingw-w64-x86_64-nlohmann-json
   ```
4. Compiler (les chemins Windows `C:\...` s'écrivent `/c/...` dans MSYS2) :
   ```bash
   cd /c/chemin/vers/SiteWatch
   cmake --preset mingw
   cmake --build --preset mingw
   ./build-mingw/SiteWatch.exe
   ```

> Toutes les DLL nécessaires (Qt **et** non-Qt : libssh2, freetype, runtime gcc…)
> sont copiées à côté de l'exe à la compilation (via `scripts/deploy-mingw.sh`),
> donc `build-mingw\SiteWatch.exe` se lance aussi bien depuis l'Explorateur
> (double-clic) que depuis le shell.

#### Compilation depuis VS Code

Le dépôt fournit une configuration `.vscode/` prête à l'emploi pour Windows +
MSYS2/MinGW :

1. Ouvrir le dossier du projet dans VS Code.
2. Installer les extensions recommandées si VS Code les propose, notamment
   **CMake Tools**.
3. Lancer **Terminal > Run Build Task...** ou `Ctrl+Shift+B`, puis choisir
   `CMake: Build (MinGW)`.
4. Pour lancer l'application compilée depuis VS Code, exécuter la tâche
   `SiteWatch: Run`.

La tâche passe par `scripts/vscode-mingw.ps1`, qui trouve MSYS2 via
`MSYS2_ROOT` ou `C:\msys64`, initialise l'environnement `MINGW64`, puis utilise
le preset CMake `mingw`.

### B — Windows avec WSL2 (Ubuntu)

Sous Windows 11, WSL affiche les fenêtres graphiques nativement (WSLg).

1. Dans un PowerShell **administrateur** : `wsl --install` puis redémarre.
2. Dans le terminal Ubuntu, suivre simplement la voie **C** ci-dessous.

### C — Linux natif

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake ninja-build \
     qt6-base-dev qt6-charts-dev \
     libssh2-1-dev zlib1g-dev nlohmann-json3-dev

cmake --preset linux
cmake --build --preset linux
./build/SiteWatch
```

Sur Linux, la config va dans `~/.config/SiteWatch` et le cache dans
`~/.local/share/SiteWatch`.

## Premier démarrage

1. Lancer `SiteWatch`.
2. Menu **Fichier → Configuration…** : ajouter les sites (serveur SFTP, utilisateur,
   clé SSH, jeton d'API cPanel pour o2switch, nom du site).
3. Cliquer **Synchroniser** : SiteWatch ouvre le pare-feu, télécharge les logs et
   analyse. Choisir la **période** en haut à droite.

La configuration et le cache sont rangés dans `%LOCALAPPDATA%\SiteWatch` sous
Windows (`~/.config` et `~/.local/share` sous Linux). Voir le
**[Guide utilisateur](GUIDE.md)** pour le détail.

## Architecture

Le projet sépare strictement le **cœur** (portable) de l'**interface** (Qt).

```
src/
├─ config/            Lecture/écriture de config.json (nlohmann-json)
├─ core/              CŒUR — indépendant de Qt
│  ├─ model/          Structures de données (LogEntry, Stats)
│  ├─ io/             GzReader — lecture des .gz (zlib)
│  ├─ parser/         ApacheLogParser — format "combined"
│  ├─ analytics/      BotDetector + StatsEngine
│  ├─ cache/          CacheManager — dossier local des logs
│  └─ net/            SftpClient — téléchargement SSH/SFTP (libssh2)
└─ ui/                Interface Qt (MainWindow, dialogues)
```

Le cœur ne connaît ni Qt ni le format d'affichage ; il compile aussi bien sous
Windows que sous Linux. Seule la couche réseau contient un `#ifdef` de plateforme.

## Crédits

Développé par **morfredus**.

Construit avec [Qt 6](https://www.qt.io), [libssh2](https://www.libssh2.org)
et [zlib](https://zlib.net).

## Licence

Copyright (C) 2026 morfredus

Ce projet est distribué sous les termes de la **GNU General Public License v3.0**.
Voir le fichier [`LICENSE`](LICENSE) pour le texte complet.
