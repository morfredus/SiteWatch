# Changelog - SiteWatch

The format is inspired by [Keep a Changelog](https://keepachangelog.com/en/).

## [Unreleased]

## [1.9.0] - 2026-07-27

### Changed

- **Configuration en deux onglets.** Le fonctionnement **local** de SiteWatch
  (stockage, sites) et les réglages **morfCollector** (adresse, heure de collecte,
  état, copies conservées) sont désormais séparés dans deux onglets distincts,
  au lieu d'être mélangés.

### Added

- **Bouton « Envoyer la configuration à morfCollector »** (onglet morfCollector) :
  enregistre puis pousse immédiatement la configuration au collecteur détecté,
  sans attendre le prochain démarrage.
- La configuration est **rechargée** après enregistrement, de sorte que les UUID
  attribués aux nouveaux sites sont disponibles immédiatement (utile pour l'envoi
  au collecteur).

## [1.8.0] - 2026-07-27

### Added

- **Heure de collecte quotidienne configurable.** Dans la configuration
  morfCollector, un champ « Collecte quotidienne à » (défaut 02:00) fixe l'heure
  à laquelle le collecteur récupère les fichiers, une fois par jour. Le réglage
  est poussé dans le manifeste (`schedule.daily_at`) ; il est donc modifiable
  depuis SiteWatch dès que morfCollector est détecté. Si le Pi était éteint à
  l'heure dite, la collecte a lieu au démarrage suivant.

## [1.7.0] - 2026-07-27

### Fixed

- **Découverte de morfCollector fiabilisée.** Le heartbeat morfBeacon n'est émis
  que toutes les ~15 s ; la découverte ponctuelle de quelques secondes le
  manquait presque toujours (morfCollector apparaissait « non détecté » alors
  qu'il tournait). SiteWatch écoute désormais les annonces **en permanence**, en
  tâche de fond, dès le démarrage (comme un superviseur) et capte la prochaine
  annonce sans bloquer l'interface.

### Added

- **Adresse du collecteur configurable** (`collectorUrl` dans la configuration).
  Renseigner par exemple `http://pi4fred:8792` connecte SiteWatch à morfCollector
  de façon déterministe, indépendamment de la découverte (utile car l'IP du Pi
  peut changer, mais son nom d'hôte reste stable). Vide = découverte automatique.
- **Gestion morfCollector dans la fenêtre de configuration** : adresse du
  collecteur, état (hôte, nombre d'objets, espace, sources), et copies conservées
  par site (lister, supprimer une sélection ou toutes les copies d'un site).
- **Gestion des copies morfCollector dans « Effacer les logs »** : à côté du cache
  local, une section permet de lister et supprimer les fichiers conservés par le
  collecteur, par site.

## [1.6.0] - 2026-07-27

### Added

- **Intégration morfCollector (côté fournisseur du contrat `morfcollect/1`).**
  Nouveau composant headless `src/collector/CollectorClient` : découverte de
  morfCollector via morfBeacon (capacité `collection`, jamais par le nom),
  vérification de compatibilité, comparaison des révisions
  (`GET /manifest/state`), envoi du manifeste (`POST /manifest`) et des secrets
  (`POST /credentials`), lecture des copies locales. Outil console
  `sitewatch-collector-sync` (Qt Core + Network, sans GUI) qui pilote cette
  synchronisation et sert à la vérifier ; la GUI réutilisera `CollectorClient`.
- **Identité stable des sites (`SiteConfig::id`, UUID).** Attribuée une fois au
  chargement si absente puis persistée. C'est l'identité d'un site vis-à-vis de
  morfCollector : elle ne dérive jamais du nom, de sorte qu'un changement d'hôte,
  de domaine ou de chemin ne casse pas l'historique de collecte.
- **Intégration morfCollector dans l'interface.** `CollectorSync` (réutilisé par
  la GUI et l'outil) construit le manifeste depuis la configuration, gère
  génération + révision (état persisté `config.json.collector.json`), pousse le
  manifeste et les secrets, détecte les sites **ajoutés** et **retirés**, et
  remplit le cache local depuis les copies du collecteur (seulement les fichiers
  absents ou de taille différente).
  - **Synchronisation à l'ouverture** : si un morfCollector est présent, la
    configuration lui est poussée automatiquement. Un site **retiré** déclenche
    une **alerte** rappelant que ses copies restent **conservées sur le Pi** (rien
    n'est effacé) ; l'effacement définitif reste une action explicite.
  - **Bouton « Tout synchroniser »** (deux modes) : *via morfCollector* (récupère
    les copies locales du Pi en une passe) ou *en direct (SFTP)* (télécharge
    depuis l'hébergeur, site par site). Également dans le menu Outils.
  - **Onglet « Copies locales »** : consulter l'état du collecteur (espace occupé,
    dernière collecte, erreurs), la liste des sites archivés (état administratif +
    opérationnel, nombre d'objets, taille), et les archives d'un site (fichier,
    période, taille). Actions déléguées au collecteur : collecter maintenant,
    suspendre / reprendre, exporter une archive, supprimer un fichier, supprimer
    toutes les copies d'un site, supprimer un site retiré. SiteWatch ne modifie
    jamais les fichiers directement.

## [1.5.2] - 2026-07-20

### Changed

- **`docs/fr/BUILD_FOR_BEGINNERS.md`**: the guide promised a `.vscode/`
  configuration shipped with the repository, but `.vscode/` is git-ignored and
  has never been committed. Beginners looked for a `CMake: Build (MinGW)` task
  and a `SiteWatch: Run` task that do not exist. The guide now uses CMake Tools
  with the `mingw` preset, which already carries the MSYS2 `PATH`.
- Version badge in `README.md` and `README.fr.md` corrected from 1.4.2 to 1.5.1.
- Updated user-facing changelog wording to use canonical production naming.

## [1.5.1] - 2026-07-19

### Changed
- **Copie vendorée de morfBeacon resynchronisée en 0.2.0** (champ `capabilities`
  du heartbeat). Ajout purement additif et facultatif : ce projet n'annonce
  aucune capacité et son comportement est strictement inchangé. La
  resynchronisation évite que la copie embarquée ne dérive de l'amont.
- **`scripts/sync-morf.sh` : résolution du dépôt source corrigée.** Le script
  cherchait exclusivement `morfBeacon` / `morfUpdate` et échouait donc sur une
  organisation où les clones portaient un suffixe de développement - c'est-à-dire
  qu'il ne fonctionnait tout simplement pas. Il accepte désormais les deux conventions.

  morfUpdate reste en 0.1.0, déjà aligné sur l'amont.

## [1.5.0] - 2026-07-13

### Added

- **LAN supervision (morfBeacon) and update check (morfUpdate).** SiteWatch now
  announces its presence on the local network (UDP heartbeat, port 45454) and
  exposes live metrics over a small local HTTP endpoint (`/status`, port 8788), so
  the running application can be watched from a central dashboard
  (RaspberryDashboard). It also checks GitHub Releases for a newer version -
  silently at startup, and on demand via **Help → "Check for updates…"**. Both are
  shared modules vendored under `third_party/morf/` (compiled into the binary, no
  external dependency). See [docs/fr/SUPERVISION_ET_MAJ.md](docs/fr/SUPERVISION_ET_MAJ.md) *(FR)*.
- **Debian packaging script** `scripts/linux/package-deb.sh`: builds a `.deb`
  from a native Linux build (x86_64 or ARM64 / Raspberry Pi), with automatic
  dependency detection, for a clean install/removal via apt. Documented in
  `docs/fr/INSTALL_LINUX.md` (Part D). The build directory is now auto-detected
  (`build/` then `build-arm64/`), so no `--build` flag is needed on Raspberry Pi.
  The package also explicitly declares `libxcb-cursor0` - required by the Qt xcb
  platform plugin since Qt 6.5 but loaded via `dlopen`, so invisible to `ldd`;
  without it the application refused to start on Raspberry Pi OS.

## [1.4.2] - 2026-07-10

### Fixed

- **UI pictograms now display universally** (Windows, Linux, WSL, Raspberry Pi).
  The interface used Unicode color emoji (KPI icons, health dots, banner, etc.),
  which are absent from default Linux/WSL fonts and poorly rendered by Qt 6.4 -
  they showed as empty “tofu” boxes. They are replaced by an **embedded icon
  font** (`resources/fonts/SiteWatchIcons.ttf`, a ~4 KB subset of Font Awesome
  Free), loaded at startup and colored via the theme. New `src/ui/Icons` module.

### Changed

- **SiteWatch is now positioned as a cross-platform Qt/C++ application** - Windows
  is one supported platform among others, not the only target. Documentation
  updated accordingly (README, README.fr).
- **Raspberry Pi 4** (Raspberry Pi OS 64-bit) added to the verified platforms,
  alongside Windows 11 and Linux Mint 22.3. This enables always-on, low-power
  deployments (Raspberry Pi, Linux VM as a scheduled task, Debian NAS, fanless
  mini-PC).

## [1.4.1] - 2026-07-10

Tooling and documentation release; no application code change.

### Added

- **Build presets for more Linux targets** (`CMakePresets.json`): `linux-arm64`
  (native ARM64, e.g. Raspberry Pi) and `linux-arm64-cross` (ARM64
  cross-compiled from x86_64, kept as a base for future CI automation). Added the
  ARM64 cross toolchain file under `cmake/toolchains/` and documented the targets
  in `docs/fr/COMPILATION.md`. No application code change.

### Changed - build strategy

- Simplified the cross-compilation strategy: building Linux x86_64 **from
  Windows** now relies on **WSL2** (native build with the `linux` preset) instead
  of a complex cross toolchain. Removed the `linux-x86_64-cross` preset and its
  toolchain file. Windows keeps a single official path, **MinGW** (MSVC is not
  supported).

### Changed

- **Documentation restructured and made bilingual.** The root project files
  (`README.md`, `CONTRIBUTING.md`, `CHANGELOG.md`, `LICENSE`, `AUTHORS`,
  `ROADMAP.md`) are now in **English**; a French README is available as
  `README.fr.md`.
- The **README was shortened** (presentation, screenshots, features, installation,
  badges + a new version badge); the remaining content was moved into `docs/`.
- The **French user documentation moved under `docs/fr/`**, and an English index
  is prepared under `docs/en/` (translations in progress). Added
  `docs/fr/ARCHITECTURE.md` (architecture, dependencies, philosophy) and
  `docs/fr/COMPILATION.md` (build from source).
- Fixed internal documentation links (including two previously broken links to
  the user guide).

## [1.4.0] - 2026-07-10

### Added

- **Smart log download assistant.** When clicking **Download logs**, SiteWatch now
  clearly distinguishes the different situations and explains them in an
  **inline banner** (non-blocking, replacing the old alert dialogs):
  - **connection failure** (host, credentials or SSH key);
  - **o2switch firewall** refused (cPanel API token);
  - **unreadable remote directory**;
  - **no log present** on the server;
  - **logs present but none matching the current filter**.
- **Automatic filter detection.** When files exist but none match, SiteWatch
  analyzes the present names, infers the common prefix (e.g. `tabacclaouey.fr`
  from `tabacclaouey.fr.ssl.log-…`) and offers a **“Use this filter”** button
  that saves it and immediately restarts the download.
- **Success banners** after a download (files fetched / already up to date),
  consistent with the light / dark / system themes.

### Changed

- New core module **`LogDiscovery`** (pure C++17, no Qt, no network, testable):
  file-to-site matching and filter detection, shared between download and
  analysis.
- **Documentation reorganization**: the user guide moved from `GUIDE.md` to
  **`docs/GUIDE.md`**; added an index **`docs/README.md`** and a troubleshooting
  guide **`docs/DEPANNAGE_LOGS.md`**.

## [1.3.1] - 2026-07-10

### Fixed

- **Linux build with Qt < 6.5** (system Qt on some distributions): the
  `QStyleHints::colorScheme()` / `setColorScheme()` APIs and the
  `Qt::ColorScheme` enum (introduced in Qt 6.5 / 6.8) are now protected by
  version guards, falling back to theme detection via the application palette.
  The “System” mode follows the OS live from Qt 6.5; on older Qt, the theme is
  determined at startup and via the menu. No behavior change on Windows.

## [1.3.0] - 2026-07-10

### Added

- **Light / dark / system themes** (menu **View → Theme**). The System mode
  automatically follows the OS appearance (Windows and Linux) and reacts to its
  changes; the choice is remembered.
- Stylesheet **externalized** into `resources/themes/` (`app.qss` with tokens
  + `light.theme` / `dark.theme` palettes), more maintainable and with no color
  hard-coded in the C++.
- **Linux** deployment: `scripts/linux/install.sh` (creates the application icon
  and copies files into the standard directories, in user or `--system` mode)
  and `scripts/linux/package-appimage.sh` (produces a self-contained **AppImage**
  to attach to releases, with no build required from the user).

### Changed

- Reorganized the `scripts/` folder into `windows/` and `linux/` subfolders.

### Fixed

- Table headers: column separators are now visible (the resize handle of the
  **Sites** tab was invisible with the default Windows theme). Contrasts revised
  to stay readable in light and dark.

## [1.2.0] - 2026-07-09

### Added

- New permanent **Sites** tab: global multi-site monitoring, state, points of
  attention, recommended action, summary and double-click to the detailed
  analysis.

### Removed

- Old **Compare sites…** dialog in the Tools menu, replaced by the permanent
  **Sites** tab.

## [1.1.2] - 2026-07-09

### Added

- Complete **VS Code** configuration: `CMake: Build (MinGW)`, `SiteWatch: Run`
  and cleanup tasks, recommended extensions and CMake Tools settings.
- Dedicated beginner guide: `docs/BUILD_FOR_BEGINNERS.md`, with MSYS2
  installation, VS Code build, common errors and a link to the user guide.

### Changed

- Project version read from the `VERSION` file by CMake.
- Automatic reconfiguration when `VERSION` changes, then rebuild with the correct
  `SITEWATCH_VERSION` value.
- CMake also declares the project headers, including Qt headers with `Q_OBJECT`,
  to make `AUTOMOC` and indexing in VS Code more reliable.
- `scripts/package-win.ps1` aligned with `VERSION`: the distribution folders and
  ZIP now use the current version by default.
- User documentation harmonized in a neutral style, without informal address or
  personal paths.

### Removed

- Old Windows **MSVC/vcpkg** path: removed the associated CMake preset and
  `vcpkg.json`. The official Windows path is now **MSYS2/MinGW**.

## [1.1.1] - 2026-07-09

### Changed

- First simplification of the Windows build around **MSYS2/MinGW**.
- Expanded build documentation in the README.
- Cleaned up frozen version references in the distribution notes.

## [1.1.0] - 2026-07-09

### Added

- **Interactive tabs** - each tabular tab (Security, WP Activity, Top pages,
  Referrers, URLs, Search) reacts to a **double-click** on a row: a detail window
  aggregates IPs, HTTP codes, user-agents, URLs, referrers, hourly breakdown and
  daily evolution.
- **Copy / export** one or several rows from any tab (clipboard or international
  CSV), on the model of the URLs tab.
- The **relevant site** is shown in the info bar of every detail window.

### Changed

- Unified detail view: the URL-specific window is replaced by a generic window
  shared by all tabs. The core classifiers (`classifyActivity`,
  `classifyReferer`) are reused to find the entries of a category or referrer -
  no duplicated logic.

## [1.0.0] - 2026-07-07

First complete release.

### Analysis

- Direct reading of compressed Apache/LiteSpeed logs (`.gz`) in the “combined”
  format.
- Bot detection classified by category (AI, search engines, SEO, other).
- Distinction between legitimate WordPress activity and real attack attempts
  (false-positive resistant), and filtering of technical resources in Top pages.
- Period filter (day, 7/30 days, month, year, custom) applied everywhere.

### Interface

- Windows 11 style, KPI cards (total, humans, bots, 404/403/500).
- **Health table** 🟢/🟠/🔴 with clickable indicators (navigation to the relevant
  tab).
- Tabs: Health, Bots (donut + %), Security, WP Activity, Top pages, Referrers,
  URLs (categories), Charts, Search.
- **Double-click detail** of a URL (IPs, user-agents, hours, referrers, codes,
  evolution).
- **Search** by IP, URL, bot, date or HTTP code.

### Network & configuration

- **SFTP download** (libssh2): incremental, filtered by site, with a progress
  bar.
- **Automatic firewall opening** on o2switch via the cPanel API.
- **SSH key** authentication (automatic fallback to password).
- Complete **graphical configuration window** (sites, cache, connection test).
- Configuration and cache stored in the standard system location
  (`%LOCALAPPDATA%\SiteWatch`, `~/.config` on Linux).

### Tools

- **Cache cleanup** by site, entirely or by period (by month).

### Portability

- Builds on **Windows** (MSYS2/MinGW) and **Linux** (GCC/Clang) - portable socket
  layer.
- Support for **other hosts** (optional firewall token, advanced log filter).
