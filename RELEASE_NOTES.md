# SiteWatch 1.4.1 — Release Notes

A **tooling and documentation** release: no change to the application itself, but
a simpler and clearer build strategy, plus a reorganized bilingual documentation.
SiteWatch remains the Apache/LiteSpeed log analyzer for the administration and
monitoring of websites hosted on o2switch (and compatible with other SSH hosts).

## What's new in 1.4.1

- **Simplified compilation chains.** Building Linux x86_64 **from Windows** no
  longer uses a fragile cross-compilation: you now use **WSL2** and build
  natively with the `linux` preset. The `linux-x86_64-cross` preset and its Qt
  sysroot toolchain were removed.
- **Kept build presets**: `mingw` (official Windows path), `linux` (x86_64,
  native or via WSL2), `linux-arm64` (native Raspberry Pi build) and
  `linux-arm64-cross` (ARM64 cross-compilation, kept as a base for future CI
  automation). MSVC is not supported.
- **Documentation restructured and bilingual.** Root files are now in English,
  with a French `README.fr.md`; the README was shortened and the rest moved under
  `docs/` (`docs/fr/` for the French guides, `docs/en/` prepared for English).

Build details (French): [docs/fr/COMPILATION.md](docs/fr/COMPILATION.md).

## Reminder — what's new in 1.4.0

- **Log download assistant.** SiteWatch now distinguishes the different causes of
  a problem and shows them in an **inline banner** (no more blocking dialog):
  connection impossible, o2switch firewall refused, unreadable remote directory,
  no log present, or logs present but not matching the filter.
- **Automatically inferred filter.** When files exist but none match, SiteWatch
  reads the present names, proposes the right **filter** (e.g. `tabacclaouey.fr`)
  and a **“Use this filter”** button that applies it and restarts the download —
  without going through the documentation.
- **Reassuring messages** on success (files downloaded or already up to date),
  adapted to the light / dark / system themes.

Step-by-step guide (French): [docs/fr/DEPANNAGE_LOGS.md](docs/fr/DEPANNAGE_LOGS.md).

## Reminder — fixed in 1.3.1

- **Linux build with Qt < 6.5** (system Qt on some distributions): theme handling
  is protected by version guards, with a fallback to palette-based detection. No
  change on Windows.

> The question SiteWatch answers is not “how many visitors?” but
> **“what really happened on my server?”**

---

## Highlights

- **Sites tab**: a global view of all sites, health priority, points of attention,
  recommended action and a summary of the sites to watch.
- **Double-click from Sites**: immediately select a site and go back to its
  detailed analysis, keeping the current period.
- **Incremental SFTP download** of `.gz` logs (only new or changed files are
  fetched), with a progress bar.
- **Automatic o2switch firewall opening** via the cPanel API before each
  connection (API token) — optional for other hosts.
- **Health table** 🟢/🟠/🔴 with clickable indicators (500 errors, attacks, 404s,
  Google activity, AI bots) leading to the relevant tab.
- **Bot detection** by category (AI, search engines, SEO, other) with a
  distribution donut and percentages.
- **Security**: distinction between legitimate WordPress activity and real attack
  attempts (false-positive resistant).
- **URL analysis** by category: all, probable attacks, WordPress operation, 404
  errors, system requests.
- **Interactive tabs**: **double-click** any row (Security, WP Activity, Top
  pages, Referrers, URLs, Search) for its full detail (IPs, user-agents, URLs,
  referrers, HTTP codes, hours, evolution), with the relevant site shown in the
  info bar; **copy** and **CSV export** of one or several rows from each tab.
- **Search** by IP, URL, bot, date or HTTP code.
- **Period filter** (day, 7/30 days, month, year, custom).
- **Cache cleanup** by site, entirely or by month.
- **Fully graphical configuration** (no manual JSON editing).

## New since 1.2.0

- **Linux deployment**: a self-contained AppImage to download from the releases
  (no build required), plus a desktop-integration script
  (`scripts/linux/install.sh`) that creates the launcher icon and installs the
  program into the standard directories. Step-by-step guide (French):
  [docs/fr/INSTALL_LINUX.md](docs/fr/INSTALL_LINUX.md).
- **Light / dark / system themes** (menu **View → Theme**). The System mode
  follows the OS appearance (Windows and Linux) and reacts to its changes; the
  choice is remembered.
- **Externalized stylesheet** (`resources/themes/`): more maintainable
  appearance, contrasts revised to stay readable in both light and dark.
- **Ergonomics fix**: table column separators are now visible (the resize handle
  of the **Sites** tab was invisible with the default Windows theme).
- Reorganized packaging scripts into `scripts/windows/` and `scripts/linux/`.

## New since 1.1.2

- Replaced the old **Compare sites…** dialog with a permanent **Sites** tab.
- Initial ranking of sites by priority: recommended intervention, to watch, then
  normal.
- Summary columns: state, points of attention, recommended action, last
  synchronization and the main counters.
- Automatic summary: most visited site, most attacked, AI bots, 404 errors,
  Google activity and best health state.

## Reminders since 1.1.2

- Ready-to-use build from **VS Code** with MSYS2/MinGW.
- Beginner guide added in `docs/fr/BUILD_FOR_BEGINNERS.md`.
- Program version centralized in the `VERSION` file.
- Windows packaging aligned with `VERSION`: the generated archive carries the
  correct version number.
- Documentation cleaned up: single Windows path **MSYS2/MinGW**, removal of the
  old MSVC/vcpkg path and of personal paths.
- CMake made more reliable to better track changes in sources, headers, resources
  and version.

## Installation

- **Windows (portable)**: unzip `SiteWatch-<version>-win64.zip` and run
  `SiteWatch.exe` — no installation, no dependency to install.
- **Linux (AppImage)**: download `SiteWatch-<version>-x86_64.AppImage`, make it
  executable (`chmod +x`) and run it — no build. Details and desktop integration:
  [docs/fr/INSTALL_LINUX.md](docs/fr/INSTALL_LINUX.md).
- **Build from source**: see [README.md](README.md) (Windows MSYS2/MinGW, or
  native Linux).

## Requirements

- **Windows** 10 / 11 (64-bit), or **Linux** 64-bit (x86_64). The AppImage may
  require FUSE 2 on some recent distributions (see the Linux guide).

> Build and runtime verified on **Windows 11** (MSYS2/MinGW, Qt 6.11) and
> **Linux Mint 22.3 “Zena”** (Ubuntu 24.04 LTS “Noble” base, Qt 6.4).
- **SSH/SFTP** access to the hosting (SSH key recommended).
- For o2switch: SSH access enabled + a **cPanel API token** for automatic firewall
  opening.

See the [user guide](docs/fr/GUIDE.md) for a step-by-step introduction.
See also the [roadmap](ROADMAP.md) for planned changes.
To understand the concrete value of the analysis, read the
[case studies](docs/fr/CASE_STUDIES.md).

## Known limitations

- Per-visitor “marketing” statistics are intentionally not covered (the tool is
  administration / security oriented).
- The SFTP download runs on the main thread: the interface freezes briefly during
  the transfer of large files.
- Bot detection and attack classification are based on known patterns: subject to
  change.

## License

SiteWatch is distributed under the **GNU GPL v3.0** license — see
[LICENSE](LICENSE).

---

*Developed by morfredus.*
