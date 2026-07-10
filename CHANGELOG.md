# Journal des versions — SiteWatch

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/).

## [1.3.1] — 2026-07-10

### Corrigé

- **Compilation sous Linux avec Qt < 6.5** (Qt système de certaines
  distributions) : les API `QStyleHints::colorScheme()` / `setColorScheme()` et
  l'énumération `Qt::ColorScheme` (introduites en Qt 6.5 / 6.8) sont désormais
  protégées par des gardes de version, avec repli sur la détection du thème via
  la palette de l'application. Le mode « Système » suit l'OS à chaud dès Qt 6.5 ;
  sur Qt plus ancien, le thème est déterminé au démarrage et via le menu.
  Aucun changement de comportement côté Windows.

## [1.3.0] — 2026-07-10

### Ajouté

- **Thèmes clair / sombre / système** (menu **Affichage → Thème**). Le mode
  Système suit automatiquement l'apparence de l'OS (Windows et Linux) et réagit
  à ses changements ; le choix est mémorisé.
- Feuille de style **externalisée** dans `resources/themes/` (`app.qss` à jetons
  + palettes `light.theme` / `dark.theme`), plus maintenable et sans couleur
  codée en dur dans le C++.
- Déploiement **Linux** : `scripts/linux/install.sh` (crée l'icône
  d'application et copie les fichiers dans les dossiers standards, en mode
  utilisateur ou `--system`) et `scripts/linux/package-appimage.sh` (produit une
  **AppImage** autonome à joindre aux releases, sans compilation côté
  utilisateur).

### Modifié

- Réorganisation du dossier `scripts/` en sous-dossiers `windows/` et `linux/`.

### Corrigé

- En-têtes de tableaux : séparateurs de colonnes désormais visibles (la poignée
  de redimensionnement de l'onglet **Sites** était invisible avec le thème
  Windows par défaut). Contrastes revus pour rester lisibles en clair et sombre.

## [1.2.0] — 2026-07-09

### Ajouté

- Nouvel onglet permanent **Sites** : supervision globale multi-sites, état,
  points d'attention, action recommandée, synthèse et double-clic vers l'analyse
  détaillée.

### Supprimé

- Ancien dialogue **Comparer les sites…** du menu Outils, remplacé par l'onglet
  permanent **Sites**.

## [1.1.2] — 2026-07-09

### Ajouté

- Configuration **VS Code** complète : tâches `CMake: Build (MinGW)`,
  `SiteWatch: Run`, nettoyage, extensions recommandées et réglages CMake Tools.
- Guide débutant dédié : `docs/BUILD_FOR_BEGINNERS.md`, avec installation
  MSYS2, compilation VS Code, erreurs fréquentes et lien vers le guide utilisateur.

### Modifié

- Version du projet lue depuis le fichier `VERSION` par CMake.
- Reconfiguration automatique quand `VERSION` change, puis recompilation avec la
  bonne valeur `SITEWATCH_VERSION`.
- CMake déclare aussi les headers du projet, y compris les headers Qt avec
  `Q_OBJECT`, pour fiabiliser `AUTOMOC` et l'indexation dans VS Code.
- Script `scripts/package-win.ps1` aligné sur `VERSION` : les dossiers et ZIP de
  distribution utilisent maintenant la version courante par défaut.
- Documentation utilisateur harmonisée en style neutre, sans tutoiement ni
  chemins personnels.

### Supprimé

- Ancienne voie Windows **MSVC/vcpkg** : suppression du preset CMake associé et
  de `vcpkg.json`. La voie Windows officielle est maintenant **MSYS2/MinGW**.

## [1.1.1] — 2026-07-09

### Modifié

- Première simplification de la compilation Windows autour de **MSYS2/MinGW**.
- Documentation de compilation enrichie dans le README.
- Nettoyage des références de version figées dans les notes de distribution.

## [1.1.0] — 2026-07-09

### Ajouté

- **Onglets interactifs** — chaque onglet tabulaire (Sécurité, Activité WP,
  Top pages, Référents, URLs, Recherche) réagit au **double-clic** sur une
  ligne : une fenêtre de détail agrège IP, codes HTTP, User-Agents, URLs,
  référents, répartition horaire et évolution par jour.
- **Copier / exporter** une ou plusieurs lignes de n'importe quel onglet
  (presse-papier ou CSV international), sur le modèle de l'onglet URLs.
- Le **site concerné** est rappelé dans la barre d'info de chaque fenêtre de détail.

### Modifié

- Détail unifié : la fenêtre spécifique aux URLs est remplacée par une fenêtre
  générique commune à tous les onglets. Les classifieurs du cœur
  (`classifyActivity`, `classifyReferer`) sont réutilisés pour retrouver les
  entrées d'une catégorie ou d'un référent — aucune logique dupliquée.

## [1.0.0] — 2026-07-07

Première version complète.

### Analyse

- Lecture directe des logs Apache/LiteSpeed compressés (`.gz`) au format « combined ».
- Détection de robots classés par catégorie (IA, moteurs de recherche, SEO, divers).
- Distinction entre activité WordPress légitime et vraies tentatives d'attaque
  (anti-faux-positifs), et filtrage des ressources techniques dans le Top pages.
- Filtre de période (jour, 7/30 jours, mois, année, personnalisé) appliqué à tout.

### Interface

- Style Windows 11, cartes KPI (total, humains, robots, 404/403/500).
- **Tableau de santé** 🟢/🟠/🔴 avec indicateurs cliquables (navigation vers l'onglet concerné).
- Onglets : Santé, Robots (donut + %), Sécurité, Activité WP, Top pages,
  Référents, URLs (catégories), Graphiques, Recherche.
- **Détail au double-clic** d'une URL (IP, User-Agents, horaires, référents, codes, évolution).
- **Recherche** par IP, URL, robot, date ou code HTTP.

### Réseau & configuration

- **Téléchargement SFTP** (libssh2) : incrémental, filtré par site, barre de progression.
- **Ouverture automatique du pare-feu** o2switch via l'API cPanel.
- Authentification par **clé SSH** (repli automatique sur mot de passe).
- **Fenêtre de configuration graphique** complète (sites, cache, test de connexion).
- Configuration et cache rangés dans l'emplacement standard du système
  (`%LOCALAPPDATA%\SiteWatch`, `~/.config` sous Linux).

### Outils

- **Nettoyage du cache** par site, en totalité ou par période (au mois).

### Portabilité

- Compile sous **Windows** (MSYS2/MinGW) et **Linux** (GCC/Clang) — couche socket portable.
- Support d'**autres hébergeurs** (jeton pare-feu optionnel, filtre de logs avancé).
