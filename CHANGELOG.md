# Changelog - SiteWatch

The format is inspired by [Keep a Changelog](https://keepachangelog.com/en/).

## [1.18.9] - 2026-09-04

### Fixed

- AppImage hardening (defensive, no functional change here). `package-appimage.sh`
  now strips any trailing CR from the `.desktop` file before handing it to
  linuxdeploy. A CRLF `.desktop` makes linuxdeploy read `Icon=sitewatch\r` and fail
  with "Could not find suitable icon for Icon entry" (the fault that hit
  ComponentHub, whose `.gitattributes` lacked the `*.desktop` LF rule SiteWatch
  already has). SiteWatch was not affected, but the shared packaging script now
  guarantees a LF desktop file regardless of how it was checked out, keeping both
  AppImage scripts identical.

## [1.18.8] - 2026-09-04

### Fixed

- AppImage, real fix on two counts. (1) The cached-tool size floor is raised to
  4 MB: a truncated 1.5 MB `linuxdeploy` had passed the old 100 KB check and failed
  with "Failed to open squashfs image"; it is now re-downloaded. (2) The icon is
  installed into `hicolor/<w>x<h>/apps/` using the PNG's real dimensions read from
  its IHDR header (no external tool), so linuxdeploy always finds a suitable icon;
  a *native* (`/usr/bin`) ImageMagick, if present, adds standard sizes -- the
  Windows `magick.exe` on the WSL PATH is never used (it cannot read `/mnt` paths).

## [1.18.7] - 2026-09-04

### Fixed

- `scripts/linux/package-appimage.sh` now validates the cached linuxdeploy tools
  (ELF magic + minimum size) and re-downloads a corrupt one instead of reusing it
  forever, fixing "Failed to open squashfs image / Failed to extract AppImage"
  from a half-written tool in the cache. The icon is also installed into
  `usr/share/pixmaps/` as a size-agnostic fallback for linuxdeploy's icon lookup.

## [1.18.6] - 2026-09-03

### Fixed

- `scripts/linux/package-deb.sh` now stages the package tree in a native temporary
  directory (`mktemp -d`) instead of under `dist/` on the current drive. On WSL the
  parc lives on `/mnt/c`, where DrvFs forces mode 777 and ignores `chmod`, so
  `dpkg-deb` refused the `DEBIAN/` control directory ("bad permissions 777, must be
  <= 0775"). Staging on the native filesystem (ext4 in WSL, the Pi's disk natively)
  fixes it; only the final `.deb` is written to `dist/`.

## [1.18.5] - 2026-08-25

### Modifié

- Resynchronisation de la copie vendorée de morfUpdate vers 0.4.8 (stratégie d'installation `source-bundle` ajoutée au moteur ; aucun changement fonctionnel côté cette application).

## [Unreleased]

## [1.18.4] - 2026-08-22

### Modifié

- Resynchroniser la copie vendorée de morfUpdate vers 0.4.3.

## [1.18.3] - 2026-08-22

### Corrigé

- Déploiement MinGW : PATH du POST_BUILD = `usr/bin` de l'install MSYS2 du compilateur + dossier de `g++` (grep/awk/ldd depuis PowerShell). `windeployqt` est aussi cherché sous `$QT_ROOT/bin`.

## [1.18.2] - 2026-08-21

### Ajouté

- Enregistrement des compilations au niveau CMake (record_compile, vendoré) : la durée de compile est signalée à morfAnalytics quel que soit le déclencheur (cmake --build direct, morf upgrade, déploiement morfDeploy).

## [1.18.1] - 2026-08-21

### Modifié

- Resynchroniser la copie vendorée de morfUpdate vers 0.4.1.

## [1.18.0] - 2026-08-21

### Ajouté

- L'écran GitHub choisit le collecteur et morfAnalytics (automatique ou
  manuel), affiche lequel est lu, et confirme l'envoi de config.
- En automatique, les analyses visent l'instance du **même hôte** que le
  collecteur (pi4dev avec pi4dev, plus le premier vu sur le réseau).

## [1.17.1] - 2026-08-21

### Corrigé

- Le magasin GitHub SQLite restait ouvert via un handle Qt local : la connexion
  se fermait à la fin de `open()`, et le redémarrage relisait une base vide.
  Le connu est rechargé dès le démarrage. morfCollector ne comble ensuite que
  les jours absents, sans relancer GitHub tout seul.
- Publication vers morfAnalytics : HTTP 405 venait d'une cible GET-only (beacon
  ou binaire trop ancien). SiteWatch vérifie `/status` avant d'envoyer l'ingest.

## [1.17.0] - 2026-08-21

### Corrigé

- La vérité GitHub est publiée vers morfAnalytics dès que le service est vu sur
  le réseau, pas seulement après un clic. Un échec d'ingest s'affiche dans
  l'écran GitHub. L'export SQLite ne mêle plus deux requêtes actives.

### Ajouté

- Tri des colonnes de l'écran GitHub. Pages, référents et assets sont conservés
  dans la vérité consolidée et transmis à l'analyse.

## [1.16.0] - 2026-08-21

### Modifié

- **SiteWatch est la source de vérité GitHub.** L'écran n'attend plus 02:30.
  **Collecter maintenant** interroge GitHub, consolide localement, complète les
  absences depuis morfCollector, puis publie vers morfAnalytics.
  **Actualiser** relit seulement le connu. Une archive collecteur ne remplace
  jamais une mesure déjà consolidée (écart journalisé).

## [1.15.1] - 2026-08-21

### Corrigé

- L'écran GitHub n'affiche plus que les dépôts **cochés** dans la configuration.
  Les autres restent dans la liste de l'onglet Configuration, mais ne sont plus
  poussés à morfCollector (plus de sources `suspended` pour les décochés).

## [1.15.0] - 2026-08-21

### Ajouté

- **Liste des dépôts GitHub.** L'onglet Configuration → GitHub interroge l'API
  pour afficher les dépôts du compte (ou de l'organisation) : on coche ceux à
  suivre au lieu de saisir les noms. Les nouveaux restent décochés. Un dépôt
  déjà suivi mais absent de la liste GitHub reste visible.
- **Envoyer la config** aussi dans les onglets Sites et GitHub (en plus de
  morfCollector). Un envoi pousse les deux familles pour que le collecteur
  tourne ensuite seul à l'heure prévue.

## [1.14.0] - 2026-08-20

### Ajouté

- **Métriques GitHub.** SiteWatch possède désormais une famille distincte des
  sites Web : liste explicite de dépôts, horaire quotidien, jeton PAT local.
  Un manifeste `github-traffic` distinct est poussé à morfCollector (instance
  `SiteWatch.github@hôte`). Le jeton n'entre jamais dans le manifeste.
- Navigation de premier niveau **Sites** / **GitHub** et page de synthèse
  (état de collecte, vues 14 j, clones, téléchargements cumulés, dernière
  release), avec lien vers morfAnalytics `/github`.

## [1.13.3] - 2026-08-20

### Corrigé

- Le build Windows retrouve Zlib depuis la toolchain MinGW active et lance le
  collecteur de DLL avec son Bash MSYS2, sans dépendre de WSL.

## [1.13.2] - 2026-08-19

### Modifié

- **Nom d'hôte dans le sélecteur de collecteur** (Configuration → morfCollector) :
  le menu des collecteurs annoncés affiche désormais le nom d'hôte annoncé (ex.
  « morfCollector (pi4fred) — http://… »), pour distinguer deux Pi d'un coup d'œil.

## [1.13.1] - 2026-08-19

### Corrigé

- **Survol de cellule illisible.** Le style natif Windows peignait la cellule
  survolée avec la couleur d'accent du système (souvent vive), rendant le texte
  illisible. Une règle `:hover` de thème impose désormais un fond doux et un texte
  contrasté (jeton `hoverBg`/`hoverText`, clair et sombre).
- **Bouton « Relancer la collecte » tronqué** dans le contrôle de cohérence : la
  hauteur de ligne par défaut coupait le libellé. Les lignes s'ajustent au contenu
  et les boutons d'action ont une hauteur minimale.

## [1.13.0] - 2026-08-19

### Ajouté

- **Contrôle de cohérence.** SiteWatch sait désormais signaler lui-même qu'un
  chemin d'alimentation ne lui donne pas le même état que la source primaire.
  Panneau dédié *Fichier → Contrôle de cohérence…* (et bouton « Cohérence » de la
  vue Sites) qui confronte, par site et par fichier, les **trois** états d'un log -
  o2switch (source primaire), objet du **collecteur sélectionné**, cache local -
  en **métadonnées seulement** (aucun `.gz` téléchargé pour le contrôle).
  - Critère : la **taille**. Les `.gz` mensuels sont append-only ; les horodatages
    ont trois sémantiques distinctes (mtime source / date de collecte / date
    d'écriture locale) et ne servent jamais de test croisé. Le hash est exclu (il
    imposerait un transfert).
  - États : `à jour`, `collecteur en retard`, `cache SiteWatch en retard`, `absent
    du collecteur`, `absent du cache`, `source o2switch inaccessible`, `collecteur
    inaccessible`, `divergence inexpliquée`. Une source injoignable est un contrôle
    **partiel**, jamais une absence ; un fichier purgé de la source mais archivé
    est « à jour (archivé) », pas une divergence.
  - **Aucune auto-réparation** : le panneau détecte, explique, et propose des
    actions déclenchées par l'utilisateur (remplir depuis le collecteur,
    télécharger en direct, relancer la collecte).
  - Multi-collecteur : le contrôle porte sur le collecteur **sélectionné** (celui
    qui alimente le cache) ; les autres collecteurs vus sont mentionnés pour
    information, sans mélange de leurs données.

### Détails techniques

- Architecture en deux couches : `CoherenceEngine` (décision **pure**, sans E/S ni
  Qt, testée par `sitewatch-coherence-selftest`, 10 cas) et `CoherenceCheck`
  (adaptateur réutilisant `SftpClient`, `CollectorClient::getObjects` et un scan du
  cache). Outil headless `sitewatch-coherence` pour la vérification en ligne de
  commande. Nécessite morfCollector ≥ 0.5.1 (lecture des objets archivés sans
  manifeste poussé).

## [1.12.0] - 2026-08-19

### Ajouté

- **Gestion de plusieurs morfCollector.** Quand plusieurs collecteurs sont
  présents sur le réseau, l'onglet *Fichier → Configuration → morfCollector*
  affiche un sélecteur « Collecteur à utiliser » : chacun peut être inspecté
  (état, sites, copies) et l'un d'eux devient la **source de lecture** épinglée.
  L'écouteur morfBeacon mémorise désormais **tous** les collecteurs vus au lieu
  du seul dernier.

### Modifié

- **« Les deux collectent, lecture d'un seul ».** SiteWatch pousse le manifeste
  (et les secrets) à **chaque** collecteur détecté - redondance d'archive - mais
  ne remplit son cache local (`fillCache`) que depuis la source de lecture
  choisie. Aucun risque de doublon de données côté SiteWatch. S'applique à la
  synchro d'ouverture, à « Tout synchroniser via collector » et au bouton
  « Envoyer la config ».

### Détails techniques

- `CollectorClient::discoverAll()` : écoute toute la fenêtre et renvoie tous les
  collecteurs (dédupliqués par hôte), au lieu de s'arrêter au premier.

## [1.11.6] - 2026-08-14

### Corrigé

- Resynchronisation de la copie vendorée de **morfBeacon**
  (`third_party/morf/beacon`) en 0.6.1, qui corrige la troncature des grandes réponses
  `/status` : `StatusServer` fermait la connexion sans drainer son tampon d'écriture,
  donc un `/status` dépassant la taille du tampon socket (~20 Ko) arrivait coupé côté
  client. On attend désormais que `bytesToWrite()` retombe à zéro avant de fermer.
  Aucun changement d'API pour SiteWatch.

## [1.11.5] - 2026-08-14

### Changed

- Resynchronisation de la copie vendorée de **morfBeacon**
  (`third_party/morf/beacon`) en 0.6.0, alignée sur le dépôt source
  (`IMetricsProvider.h`, `StatusServer.cpp`). Aucun changement de comportement.

## [1.11.4] - 2026-08-01

### Added

- La synthèse destinée à morfAnalytics inclut les séries quotidiennes de trafic
  humain, robots IA/SEO et activité normale. Elles permettent les analyses
  temporelles avancées sans transférer les journaux source.

## [1.11.3] - 2026-07-31

### Fixed

- Les classements envoyés à morfAnalytics sont bornés aux informations les plus
  utiles. Une analyse avec un très grand nombre d'URL ne peut plus dépasser la
  taille admise par le service et disparaître de la page d'analyses.

### Changed

- Les évolutions journalières transmises couvrent les 90 derniers jours, tandis
  que chaque classement conserve ses 20 éléments les plus significatifs.

## [1.11.2] - 2026-07-31

### Fixed

- Le bouton **Analyses avancées** republie la synthèse courante avant d'ouvrir
  morfAnalytics. Cela évite une page vide lorsqu'elle est ouverte avant la fin
  de la publication automatique.

### Added

- Les synthèses transmises à morfAnalytics incluent les pages les plus visitées,
  les pages ciblées, les robots actifs et leur évolution journalière.

## [1.11.1] - 2026-07-31

### Fixed

- Publication vers morfAnalytics fiabilisée : corps explicitement JSON et
  republication automatique de la dernière analyse quand le service est détecté
  après coup. Un échec est signalé dans la barre d'état.

## [1.11.0] - 2026-07-31

### Added

- **Intégration morfAnalytics.** SiteWatch détecte le service sur le réseau,
  publie automatiquement la synthèse d'un site après analyse et propose le
  bouton **Analyses avancées**. Il reste grisé avec une infobulle explicite tant
  que morfAnalytics n'est pas disponible.

## [1.10.0] - 2026-07-28

### Added

- **Panneau d'état morfCollector** dans la configuration (onglet *morfCollector*),
  qui occupe la partie basse jusque-là vide :
  - synthèse du service (hôte, version, heure de collecte quotidienne, nombre de
    fichiers téléchargés et conservés, taille, date de dernière collecte) ;
  - **tableau des sites confiés** au collecteur, avec pour chacun son état côté
    collecteur (*prêt*, *à jour*, *identifiants refusés*, *serveur injoignable*…),
    le nombre de fichiers téléchargés et la taille. L'erreur détaillée s'affiche
    en infobulle. Le panneau se remplit automatiquement à l'ouverture quand un
    collecteur est déjà connu.
- **Bouton « Collecter maintenant »** : déclenche une collecte immédiate de tous
  les sites, sans attendre l'heure programmée.
- **Bouton « Réinitialiser »** : efface les copies conservées sur le collecteur
  puis les re-télécharge depuis l'hébergeur (utile si des fichiers manquent). Le
  cache local de SiteWatch n'est pas touché.

### Fixed

- **Port `/status` déplacé de 8788 à 8881** pour éviter toute collision. 8788 est
  le port du service **morfSensor** ; SiteWatch le reprenait, ce qui pouvait
  entrer en conflit sur une machine hébergeant les deux. Le port vit désormais
  dans la plage réservée aux applications de bureau (`appRange` 8880-8899, hors du
  bloc des services), enregistrée dans `morfTools/ecosystem.json`.
- **Sources bloquées en « identifiants refusés » après un redéploiement du
  collecteur.** Une fois sa synchronisation initiale enregistrée, SiteWatch ne
  redéposait plus jamais les secrets, alors que le coffre du collecteur pouvait
  avoir été vidé (réinstallation, changement de machine) : les sources restaient
  bloquées sans espoir de reprise automatique. La synchronisation compare
  désormais le nombre de secrets présents côté collecteur au nombre de sites, et
  les **redépose d'office si le coffre est incomplet** (auto-guérison). Les
  boutons « Collecter maintenant » et « Réinitialiser » redéposent aussi les
  secrets par sécurité avant de lancer une collecte.

## [1.9.2] - 2026-07-28

### Changed

- **Menus réorganisés** selon les conventions habituelles : **Fichier /
  Analyse / Affichage / Aide**. Tout est désormais accessible depuis les menus :
  - *Analyse* : « Télécharger les logs du site » (Ctrl+D), « Analyser le site »
    (F5), « Rechercher… » (Ctrl+F) et « Tout synchroniser » ▸ (via morfCollector /
    en direct SFTP) ;
  - *Affichage → Aller à* : ouvre n'importe quel onglet, y compris **Copies
    locales**, au clavier comme à la souris.
- **Libellés unifiés.** L'ancien « Synchroniser maintenant » (menu) et le bouton
  « Télécharger les logs » désignaient la même action sous deux noms : c'est
  désormais partout « Télécharger les logs du site » (site courant), distinct de
  « Tout synchroniser » (tous les sites).

### Fixed

- **Aide et « À propos » à jour** avec la version actuelle : morfCollector, onglet
  Copies locales, « Tout synchroniser », nouveaux raccourcis clavier.

## [1.9.1] - 2026-07-27

### Fixed

- **Liaison avec morfCollector qui se perdait** après quelques rafraîchissements
  (le collecteur devenait « non détecté », un redémarrage rétablissait). Cause :
  chaque découverte ponctuelle **rebindait le port UDP 45454**, ce qui, sous
  Windows, cassait l'écoute des annonces. Il n'y a désormais **qu'un seul écouteur
  permanent** (fenêtre principale) ; les dialogues reçoivent l'adresse déjà connue
  et ne relancent plus jamais de découverte.

### Changed

- **Onglet morfCollector, boutons clarifiés.** Le bouton collé à l'adresse est
  renommé **« Se connecter »** (il vérifie l'adresse et affiche l'état, il ne
  modifie pas l'adresse). Le bouton **« Envoyer la config »** devient compact et
  se place à côté. Un bouton dédié **« Rafraîchir les fichiers »** est ajouté dans
  la liste des copies ; l'actualisation de l'état et celle des fichiers sont
  séparées. La suppression de fichiers demande toujours une confirmation.

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
