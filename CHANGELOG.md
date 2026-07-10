# Changelog — SiteWatch

The format is inspired by [Keep a Changelog](https://keepachangelog.com/en/).

## [1.4.1] — 2026-07-10

Tooling and documentation release; no application code change.

### Added

- **Build presets for more Linux targets** (`CMakePresets.json`): `linux-arm64`
  (native ARM64, e.g. Raspberry Pi) and `linux-arm64-cross` (ARM64
  cross-compiled from x86_64, kept as a base for future CI automation). Added the
  ARM64 cross toolchain file under `cmake/toolchains/` and documented the targets
  in `docs/fr/COMPILATION.md`. No application code change.

### Changed — build strategy

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

## [1.4.0] — 2026-07-10

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

## [1.3.1] — 2026-07-10

### Fixed

- **Linux build with Qt < 6.5** (system Qt on some distributions): the
  `QStyleHints::colorScheme()` / `setColorScheme()` APIs and the
  `Qt::ColorScheme` enum (introduced in Qt 6.5 / 6.8) are now protected by
  version guards, falling back to theme detection via the application palette.
  The “System” mode follows the OS live from Qt 6.5; on older Qt, the theme is
  determined at startup and via the menu. No behavior change on Windows.

## [1.3.0] — 2026-07-10

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

## [1.2.0] — 2026-07-09

### Added

- New permanent **Sites** tab: global multi-site monitoring, state, points of
  attention, recommended action, summary and double-click to the detailed
  analysis.

### Removed

- Old **Compare sites…** dialog in the Tools menu, replaced by the permanent
  **Sites** tab.

## [1.1.2] — 2026-07-09

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

## [1.1.1] — 2026-07-09

### Changed

- First simplification of the Windows build around **MSYS2/MinGW**.
- Expanded build documentation in the README.
- Cleaned up frozen version references in the distribution notes.

## [1.1.0] — 2026-07-09

### Added

- **Interactive tabs** — each tabular tab (Security, WP Activity, Top pages,
  Referrers, URLs, Search) reacts to a **double-click** on a row: a detail window
  aggregates IPs, HTTP codes, user-agents, URLs, referrers, hourly breakdown and
  daily evolution.
- **Copy / export** one or several rows from any tab (clipboard or international
  CSV), on the model of the URLs tab.
- The **relevant site** is shown in the info bar of every detail window.

### Changed

- Unified detail view: the URL-specific window is replaced by a generic window
  shared by all tabs. The core classifiers (`classifyActivity`,
  `classifyReferer`) are reused to find the entries of a category or referrer —
  no duplicated logic.

## [1.0.0] — 2026-07-07

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

- Builds on **Windows** (MSYS2/MinGW) and **Linux** (GCC/Clang) — portable socket
  layer.
- Support for **other hosts** (optional firewall token, advanced log filter).
