# Journal des versions — SiteWatch

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/).

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

- **Comparaison de sites** sur une même période.
- **Nettoyage du cache** par site, en totalité ou par période (au mois).

### Portabilité

- Compile sous **Windows** (MSVC) et **Linux** (GCC/Clang) — couche socket portable.
- Support d'**autres hébergeurs** (jeton pare-feu optionnel, filtre de logs avancé).
