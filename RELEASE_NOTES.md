# SiteWatch 1.1.2 — Notes de version

Version de maintenance et de simplification de **SiteWatch**, analyseur de logs
Apache/LiteSpeed pour l'administration et la supervision de sites web hébergés
sur o2switch (et compatible avec d'autres hébergeurs SSH).

> La question à laquelle SiteWatch répond n'est pas « combien de visiteurs ? »
> mais **« que s'est-il réellement passé sur mon serveur ? »**

---

## Points forts

- **Téléchargement SFTP incrémental** des logs `.gz` (seuls les fichiers
  nouveaux ou modifiés sont récupérés), avec barre de progression.
- **Ouverture automatique du pare-feu o2switch** via l'API cPanel avant chaque
  connexion (jeton d'API) — optionnelle pour les autres hébergeurs.
- **Tableau de santé** 🟢/🟠/🔴 avec indicateurs cliquables (erreurs 500,
  attaques, 404, activité Google, robots IA) menant à l'onglet concerné.
- **Détection de robots** par catégorie (IA, moteurs, SEO, divers) avec donut
  de répartition et pourcentages.
- **Sécurité** : distinction entre activité WordPress légitime et vraies
  tentatives d'attaque (anti-faux-positifs).
- **Analyse des URLs** par catégorie : toutes, attaques probables, fonctionnement
  WordPress, erreurs 404, requêtes système.
- **Onglets interactifs** : **double-clic** sur n'importe quelle ligne (Sécurité,
  Activité WP, Top pages, Référents, URLs, Recherche) pour son détail complet
  (IP, User-Agents, URLs, référents, codes HTTP, horaires, évolution), avec le
  site concerné rappelé en barre d'info ; **copie** et **export CSV** d'une ou
  plusieurs lignes depuis chaque onglet.
- **Recherche** par IP, URL, robot, date ou code HTTP.
- **Comparaison de sites** sur une même période.
- **Filtre de période** (jour, 7/30 jours, mois, année, personnalisé).
- **Nettoyage du cache** par site, en totalité ou par mois.
- **Configuration entièrement graphique** (aucune édition manuelle de JSON).

## Nouveautés depuis 1.1.0

- Compilation depuis **VS Code** prête à l'emploi avec MSYS2/MinGW.
- Guide débutant ajouté dans `docs/BUILD_FOR_BEGINNERS.md`.
- Version du programme centralisée dans le fichier `VERSION`.
- Packaging Windows aligné sur `VERSION` : l'archive générée porte le bon numéro
  de version.
- Documentation nettoyée : voie Windows unique **MSYS2/MinGW**, suppression de
  l'ancien chemin MSVC/vcpkg et des chemins personnels.
- CMake fiabilisé pour mieux suivre les changements de sources, headers,
  ressources et version.

## Installation

- **Version portable** : décompresser `SiteWatch-<version>-win64.zip` et lancer
  `SiteWatch.exe` — aucune installation, aucune dépendance à installer.
- **Compilation depuis les sources** : voir [README.md](README.md)
  (MSYS2/MinGW, WSL2, ou Linux natif).

## Configuration requise (version portable)

- Windows 10 / 11 (64 bits).
- Un accès **SSH/SFTP** à l'hébergement (clé SSH recommandée).
- Pour o2switch : l'accès SSH activé + un **jeton d'API cPanel** pour
  l'ouverture automatique du pare-feu.

Voir le [Guide utilisateur](GUIDE.md) pour la prise en main pas à pas.

## Limitations connues

- Les statistiques par visiteur « marketing » ne sont volontairement pas
  couvertes (l'outil est orienté administration / sécurité).
- Le téléchargement SFTP s'exécute dans le fil principal : l'interface se fige
  brièvement pendant le transfert de gros fichiers.
- Détection de robots et classification d'attaques basées sur des motifs
  connus : susceptibles d'évoluer.

## Licence

SiteWatch est distribué sous licence **GNU GPL v3.0** — voir [LICENSE](LICENSE).

---

*Développé par morfredus.*
