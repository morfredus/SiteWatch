# Contributing to SiteWatch

Thanks for your interest in SiteWatch. This document explains how to build the
project, report a bug, propose a change, and the coding conventions to follow.

---

## Building the project

The full details are in the [README](README.md) and in
[docs/fr/COMPILATION.md](docs/fr/COMPILATION.md). In short, on Windows the
simplest path is **MSYS2 / MinGW**:

```bash
# in the "MSYS2 MINGW64" shell
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts \
  mingw-w64-x86_64-libssh2 mingw-w64-x86_64-zlib mingw-w64-x86_64-nlohmann-json

cd /c/path/to/SiteWatch
cmake --preset mingw
cmake --build --preset mingw
```

Other paths (WSL2, native Linux) are described in the documentation.
The code must stay **portable**: it builds on Windows (MinGW) and Linux.

**Vendored common modules (`third_party/morf/`).** The LAN supervision
(morfBeacon) and update-check (morfUpdate) libraries are **copies** compiled into
the binary — no external repository is needed to build. Do **not** edit them under
`third_party/`: change the source in the `morfBeacon_travail` / `morfUpdate_travail`
repositories, then resync with `scripts/sync-morf.ps1` (or `scripts/sync-morf.sh`).
See [docs/fr/SUPERVISION_ET_MAJ.md](docs/fr/SUPERVISION_ET_MAJ.md) *(FR)*.

---

## Reporting a bug

Open an **issue** on the project's GitHub repository, including:

- the **version** of SiteWatch (menu *Help → About*, or the `VERSION` file);
- the **operating system** used (Windows 10/11, Linux + distribution);
- the **steps to reproduce** the problem;
- the **expected** behavior versus the observed behavior;
- if possible, a **screenshot** and the exact error message.

⚠️ Never attach `config.json`, passwords, SSH keys or API tokens.
Anonymize any sensitive data.

---

## Proposing a change (Pull Request)

1. **Fork** the repository and create a descriptive branch
   (`fix/parser-error`, `feat/json-export`…).
2. **Build and test** the change (at least the MinGW path).
3. Make **clear commits** (one idea per commit, imperative message:
   "Add filtering by HTTP code").
4. Respect the **architecture** and the **coding conventions** below.
5. Open the **Pull Request** against the main branch with a description of what
   the change does and why.

Every new source file must carry the license header:

```cpp
/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */
```

Every contribution is distributed under the project's license (**GNU GPL v3.0**).

---

## Coding conventions

**Architecture** — the separation is strict:

- `src/core/`: the business **core** (parser, statistics, cache, network). It must
  **never** include Qt or depend on the interface — it stays portable.
- `src/config/`: reading/writing the configuration.
- `src/ui/`: the **interface** (Qt). The only place allowed to use Qt.

**Language & style**

- **C++17**, CMake ≥ 3.21.
- Indentation: **4 spaces**, no tabs.
- K&R-style braces (opening brace on the same line).
- Naming: classes in `PascalCase`, methods and variables in `camelCase`,
  **members suffixed with `_`** (e.g. `siteSelector_`), constants `kMyConstant`.
- **Comments in French**, concise and useful (the "why", not the "what").
- Source files encoded in **UTF-8**; user-visible strings are in French.
- Prefer standard-library types and containers in `core/`, and Qt types in `ui/`.

**Quality**

- One class = one responsibility; each module must be able to evolve on its own.
- No hard-coded secret in the code (passwords, tokens): everything goes through
  `config.json` (ignored by Git).
- The project must keep building **without warnings** on MinGW.

---

*Developed by morfredus.*
