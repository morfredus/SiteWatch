# SiteWatch 1.4.0 — Notes de version

Version qui rend le **téléchargement des logs beaucoup plus intelligent** :
SiteWatch explique désormais clairement ce qui se passe et vous guide au lieu de
laisser un simple message discret. SiteWatch reste l'analyseur de logs
Apache/LiteSpeed pour l'administration et la supervision de sites web hébergés
sur o2switch (et compatible avec d'autres hébergeurs SSH).

## Nouveautés dans 1.4.0

- **Assistant de téléchargement des logs.** SiteWatch distingue maintenant les
  différentes causes d'un problème et les affiche dans une **bannière intégrée**
  (plus de fenêtre bloquante) : connexion impossible, pare-feu o2switch refusé,
  dossier distant illisible, aucun log présent, ou logs présents mais ne
  correspondant pas au filtre.
- **Filtre déduit automatiquement.** Quand des fichiers existent mais qu'aucun ne
  correspond, SiteWatch lit les noms présents, propose le bon **filtre** (ex.
  `tabacclaouey.fr`) et un bouton **« Utiliser ce filtre »** qui l'applique et
  relance le téléchargement — sans passer par la documentation.
- **Messages rassurants** en cas de succès (fichiers téléchargés ou déjà à jour),
  adaptés aux thèmes clair / sombre / système.

Guide pas à pas : [docs/DEPANNAGE_LOGS.md](docs/DEPANNAGE_LOGS.md).

## Rappel — corrigé dans 1.3.1

- **Compilation sous Linux avec Qt < 6.5** (Qt système de certaines
  distributions) : la gestion du thème est protégée par des gardes de version,
  avec repli sur la détection via la palette. Aucun changement côté Windows.

> La question à laquelle SiteWatch répond n'est pas « combien de visiteurs ? »
> mais **« que s'est-il réellement passé sur mon serveur ? »**

---

## Points forts

- **Nouvel onglet Sites** : vue globale de tous les sites, priorité de santé,
  points d'attention, action recommandée et synthèse des sites à surveiller.
- **Double-clic depuis Sites** : sélectionner immédiatement un site et revenir à
  son analyse détaillée, en conservant la période courante.
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
- **Filtre de période** (jour, 7/30 jours, mois, année, personnalisé).
- **Nettoyage du cache** par site, en totalité ou par mois.
- **Configuration entièrement graphique** (aucune édition manuelle de JSON).

## Nouveautés depuis 1.2.0

- **Déploiement Linux** : AppImage autonome à télécharger dans les releases
  (aucune compilation), plus un script d'intégration au bureau
  (`scripts/linux/install.sh`) qui crée l'icône de lancement et installe le
  programme dans les dossiers standards. Guide pas à pas :
  [docs/INSTALL_LINUX.md](docs/INSTALL_LINUX.md).
- **Thèmes clair / sombre / système** (menu **Affichage → Thème**). Le mode
  Système suit l'apparence de l'OS (Windows et Linux) et réagit à ses
  changements ; le choix est mémorisé.
- **Feuille de style externalisée** (`resources/themes/`) : apparence plus
  maintenable, contrastes revus pour rester lisibles en clair comme en sombre.
- **Correctif d'ergonomie** : les séparateurs de colonnes des tableaux sont
  désormais visibles (la poignée de redimensionnement de l'onglet **Sites**
  était invisible avec le thème Windows par défaut).
- Réorganisation des scripts de packaging en `scripts/windows/` et
  `scripts/linux/`.

## Nouveautés depuis 1.1.2

- Remplacement de l'ancien dialogue **Comparer les sites…** par un onglet
  permanent **Sites**.
- Classement initial des sites par priorité : intervention recommandée, à
  surveiller, puis normal.
- Colonnes synthétiques : état, points d'attention, action recommandée, dernière
  synchronisation et principaux compteurs.
- Synthèse automatique : site le plus visité, le plus attaqué, robots IA,
  erreurs 404, activité Google et meilleur état de santé.

## Rappels depuis 1.1.2

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

- **Windows (portable)** : décompresser `SiteWatch-<version>-win64.zip` et lancer
  `SiteWatch.exe` — aucune installation, aucune dépendance à installer.
- **Linux (AppImage)** : télécharger `SiteWatch-<version>-x86_64.AppImage`, la
  rendre exécutable (`chmod +x`) et la lancer — aucune compilation. Détails et
  intégration au bureau : [docs/INSTALL_LINUX.md](docs/INSTALL_LINUX.md).
- **Compilation depuis les sources** : voir [README.md](README.md)
  (Windows MSYS2/MinGW, ou Linux natif).

## Configuration requise

- **Windows** 10 / 11 (64 bits), ou **Linux** 64 bits (x86_64). L'AppImage peut
  nécessiter FUSE 2 sur certaines distributions récentes (voir le guide Linux).

> Compilation et fonctionnement vérifiés sous **Windows 11** (MSYS2/MinGW,
> Qt 6.11) et **Linux Mint 22.3 « Zena »** (base Ubuntu 24.04 LTS « Noble »,
> Qt 6.4).
- Un accès **SSH/SFTP** à l'hébergement (clé SSH recommandée).
- Pour o2switch : l'accès SSH activé + un **jeton d'API cPanel** pour
  l'ouverture automatique du pare-feu.

Voir le [Guide utilisateur](docs/GUIDE.md) pour la prise en main pas à pas.
Voir aussi la [roadmap](ROADMAP.md) pour les évolutions envisagées.
Pour comprendre l'intérêt concret de l'analyse, consulter les
[études de cas](docs/CASE_STUDIES.md).

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
