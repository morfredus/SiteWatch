# SiteWatch

[![GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake)
![Status](https://img.shields.io/badge/Status-Active-success)

> ✅ **Compilation et fonctionnement vérifiés** sous **Windows 11** (MSYS2/MinGW, Qt 6.11)
> et **Linux Mint 22.3 « Zena »** — base Ubuntu 24.04 LTS « Noble », Qt 6.4.

**Desktop investigation tool for Apache and LiteSpeed access logs**

SiteWatch est une application de bureau dédiée à l'administration et à la supervision de sites web.

Contrairement aux outils de statistiques traditionnels, SiteWatch n'a pas pour objectif de compter les visiteurs ou de produire des rapports marketing.

Son objectif est beaucoup plus simple :

> **Comprendre ce qui s'est réellement passé sur un serveur web.**

L'application télécharge les journaux Apache ou LiteSpeed via SFTP, les analyse localement et présente les résultats sous une forme immédiatement exploitable.

Aucune base SQL.

Aucune dépendance WordPress.

Aucun plugin à installer sur les sites.

Lecture directe des fichiers de logs compressés (`.gz`).

SiteWatch fonctionne avec tout hébergeur proposant un accès SSH/SFTP et dispose d'une intégration avancée pour **o2switch** (ouverture automatique du pare-feu via l'API cPanel).

---

## Pourquoi SiteWatch ?

Les outils classiques répondent généralement à des questions comme :

- Combien ai-je eu de visiteurs ?
- Quelle est ma page la plus consultée ?
- Quelle est la provenance de mon trafic ?

SiteWatch répond plutôt à :

- Pourquoi cette URL retourne-t-elle autant de 404 ?
- Quel robot scanne actuellement mon site ?
- Pourquoi Google demande-t-il cette ressource ?
- Cette activité WordPress est-elle normale ?
- Est-ce une vraie attaque ou un faux positif ?
- Quel site nécessite mon intervention aujourd'hui ?

SiteWatch est conçu comme un **outil d'investigation** destiné aux administrateurs de sites web.

---

## Documentation

- 📖 **Guide utilisateur** : `GUIDE.md`
- 🐧 **Installer et lancer sous Linux** : `docs/INSTALL_LINUX.md`
- 🛠 **Compiler SiteWatch quand on débute (Windows)** : `docs/BUILD_FOR_BEGINNERS.md`
- 📚 **Études de cas** : `docs/CASE_STUDIES.md`
- 🗺 **Roadmap** : `ROADMAP.md`
- 🤝 **Contribuer** : `CONTRIBUTING.md`
- 📝 **Journal des versions** : `CHANGELOG.md`

---

# Aperçu

## Tableau de bord

Après chaque synchronisation ou analyse, SiteWatch affiche automatiquement une vue globale de l'ensemble des sites configurés.

L'objectif est de permettre à l'administrateur d'identifier en quelques secondes les sites qui nécessitent une attention particulière avant d'accéder aux analyses détaillées.

Le tableau de bord affiche notamment :

- état global de chaque site ;
- indicateurs principaux ;
- points d'attention ;
- action recommandée ;
- date de la dernière synchronisation.

Un double-clic sur un site le sélectionne automatiquement et ouvre immédiatement son analyse détaillée.

![Tableau de bord](docs/pictures/dashboard.png)

---

## Santé

Le tableau Santé synthétise les principaux indicateurs du site sélectionné.

Les anomalies sont immédiatement visibles grâce à un système d'états 🟢 🟠 🔴 et permettent d'accéder directement à l'onglet concerné.

![Santé](docs/pictures/health.png)

---

## Robots

SiteWatch classe automatiquement les robots par catégorie :

- Intelligence artificielle
- Moteurs de recherche
- SEO
- Robots divers

Le graphique permet d'obtenir instantanément une vision de leur répartition.

![Robots](docs/pictures/robots.png)

---

## Sécurité

Les requêtes suspectes sont regroupées automatiquement afin de distinguer rapidement l'activité normale des véritables tentatives d'attaque.

Les faux positifs liés au fonctionnement normal de WordPress sont éliminés autant que possible.

![Sécurité](docs/pictures/security.png)

---

## Activité WordPress

SiteWatch identifie automatiquement les principales activités liées à WordPress afin de distinguer le fonctionnement normal d'un site des comportements inhabituels.

Sont notamment reconnus :

- Administration (`/wp-admin/`)
- Connexion (`wp-login.php`)
- REST API
- `admin-ajax.php`
- XML-RPC
- Cron WordPress
- Uploads
- Plugins
- Thèmes

L'objectif n'est pas de compter les requêtes WordPress mais de comprendre leur origine et leur impact.

![Activité WordPress](docs/pictures/wordpress-activity.png)

---

## Top Pages

Les pages les plus demandées sont regroupées afin d'identifier rapidement :

- les contenus les plus consultés ;
- les fichiers statiques les plus sollicités ;
- les ressources anormalement demandées.

Un double-clic ouvre une analyse complète de la ressource sélectionnée.

![Top Pages](docs/pictures/top-pages.png)

---

## Référents

Les référents sont automatiquement regroupés afin de distinguer :

- moteurs de recherche ;
- réseaux sociaux ;
- accès directs ;
- autres sites web.

Cette vue permet notamment de détecter rapidement des référents inhabituels ou des campagnes de liens.

![Référents](docs/pictures/referrers.png)

---

## Analyse des URLs

Toutes les URLs peuvent être explorées ou filtrées selon plusieurs catégories.

Par exemple :

- Toutes les URLs
- Attaques probables
- Erreurs 404
- WordPress
- Robots
- Système

Chaque ligne représente une URL unique regroupant l'ensemble des accès correspondants.

Un double-clic ouvre une fenêtre d'analyse détaillée.

![URLs](docs/pictures/urls.png)

---

## Fenêtres de détail

La plupart des listes présentes dans SiteWatch sont interactives.

Un double-clic ouvre une fenêtre regroupant toutes les informations concernant l'élément sélectionné.

Selon le contexte, cette fenêtre peut afficher :

- adresses IP ;
- User-Agents ;
- codes HTTP ;
- référents ;
- répartition horaire ;
- évolution quotidienne ;
- fréquence d'apparition ;
- sites concernés.

Toutes les listes disposent des mêmes fonctionnalités :

- copie dans le presse-papiers ;
- export CSV ;
- tri des colonnes ;
- sélection multiple.

L'objectif est d'offrir une navigation cohérente dans toute l'application.

![Fenêtre de détail](docs/pictures/detail-window.png)

---

## Graphiques

Plusieurs graphiques permettent de suivre l'évolution des principaux indicateurs sur la période analysée.

Ils facilitent la détection de tendances ou de changements importants.

Les graphiques portent notamment sur :

- trafic humain ;
- robots ;
- robots IA ;
- Google ;
- erreurs HTTP ;
- activité WordPress.

![Graphiques](docs/pictures/graph.png)

---

# Fonctionnalités principales

- Analyse locale des journaux Apache et LiteSpeed
- Téléchargement incrémental via SFTP
- Gestion de plusieurs sites
- Tableau de bord multi-sites
- Tableau de santé interactif
- Détection automatique des robots
- Détection des robots d'intelligence artificielle
- Analyse des moteurs de recherche
- Détection des attaques courantes
- Analyse de l'activité WordPress
- Analyse des URLs
- Analyse des référents
- Top Pages
- Recherche avancée
- Fenêtres de détail interactives
- Copie des résultats
- Export CSV
- Cache local
- Nettoyage du cache
- Configuration graphique
- Thèmes clair / sombre / système (suivi automatique de l'apparence de l'OS)
- Journalisation
- API o2switch pour l'ouverture automatique du pare-feu

---

# Hébergement

SiteWatch fonctionne avec tout hébergeur proposant un accès SSH/SFTP.

Une intégration spécifique est disponible pour **o2switch**, permettant notamment l'ouverture automatique du pare-feu via l'API cPanel avant le téléchargement des journaux.

---

# Installation

Aucun composant n'est à installer sur les serveurs analysés.

SiteWatch fonctionne exclusivement à partir des journaux Apache ou LiteSpeed téléchargés via SFTP.

Le logiciel est portable.

### Windows

Il suffit d'extraire l'archive `SiteWatch-<version>-win64.zip` et d'exécuter :

```
SiteWatch.exe
```

Aucune installation n'est nécessaire.

### Linux

La solution la plus simple est l'**AppImage** : un fichier unique et autonome,
sans compilation ni dépendance à installer. Après téléchargement depuis les
releases :

```bash
chmod +x SiteWatch-<version>-x86_64.AppImage
./SiteWatch-<version>-x86_64.AppImage
```

Pour ajouter l'icône au menu des applications, ou pour compiler puis intégrer
SiteWatch au bureau, voir le guide dédié :
**[docs/INSTALL_LINUX.md](docs/INSTALL_LINUX.md)**.

---

# Premier démarrage

## 1. Ajouter un site

Créer un nouveau site en renseignant :

- Nom du site
- Adresse du serveur
- Port SSH
- Nom d'utilisateur
- Méthode d'authentification (mot de passe ou clé SSH)
- Répertoire contenant les journaux

Pour les hébergements **o2switch**, il est possible d'activer automatiquement l'ouverture du pare-feu via l'API cPanel avant chaque synchronisation.

---

## 2. Synchroniser les journaux

Une synchronisation télécharge uniquement les nouveaux fichiers ou les nouvelles données disponibles.

Les téléchargements sont incrémentaux afin de limiter les transferts.

Les journaux sont ensuite stockés dans le cache local de SiteWatch.

---

## 3. Choisir une période

Une fois les journaux disponibles, sélectionner la période souhaitée.

Par exemple :

- Aujourd'hui
- Hier
- Les 7 derniers jours
- Les 30 derniers jours
- Période personnalisée

Toutes les analyses utilisent automatiquement cette période.

---

## 4. Lancer l'analyse

Après analyse, SiteWatch affiche automatiquement le tableau de bord multi-sites.

Cette vue permet d'identifier immédiatement :

- les sites nécessitant une attention ;
- les anomalies importantes ;
- les actions recommandées.

Un double-clic ouvre directement l'analyse détaillée du site sélectionné.

---

# Workflow recommandé

Le fonctionnement normal de SiteWatch est volontairement simple.

1. Synchroniser les journaux.
2. Consulter le tableau de bord.
3. Identifier les sites nécessitant une intervention.
4. Ouvrir l'analyse détaillée.
5. Explorer les informations grâce aux doubles-clics.
6. Exporter les résultats si nécessaire.

Cette approche permet de passer rapidement d'une vue globale à une investigation très précise.

---

# Recherche

L'onglet Recherche permet de retrouver rapidement une information dans l'ensemble des journaux analysés.

Selon les versions, il est possible de rechercher notamment :

- une adresse IP ;
- une URL ;
- un User-Agent ;
- un référent ;
- un code HTTP ;
- un mot-clé.

Les résultats peuvent ensuite être explorés grâce aux fenêtres de détail.

---

# Export

La plupart des tableaux de SiteWatch proposent deux fonctions communes :

## Copier

Copie la sélection dans le presse-papiers afin de pouvoir la coller directement dans un rapport, un courriel ou un document.

## Export CSV

Exporte les lignes sélectionnées au format CSV compatible avec Excel, LibreOffice Calc ou tout autre tableur.

Les exports utilisent un format international afin de faciliter leur réutilisation.

---

# Configuration

L'ensemble de la configuration est accessible depuis l'interface graphique.

Elle permet notamment de gérer :

- les sites surveillés ;
- les paramètres SSH ;
- les clés privées ;
- les options o2switch ;
- le répertoire du cache ;
- les préférences de l'application.

Aucune modification manuelle des fichiers de configuration n'est normalement nécessaire.

---

# Compilation

## Windows

La méthode officielle repose sur **MSYS2**, **MinGW** et **VS Code**.

L'installation des dépendances s'effectue avec une seule commande :

```bash
pacman -S --needed mingw-w64-x86_64-{gcc,cmake,ninja,qt6-base,qt6-charts,libssh2,zlib,nlohmann-json}
```

Compilation :

```bash
cmake --preset mingw
cmake --build --preset mingw
```

Exécution :

```bash
./build-mingw/SiteWatch.exe
```

Le guide détaillé est disponible dans :

```
docs/BUILD_FOR_BEGINNERS.md
```

---

## Linux

La compilation native Linux est fonctionnelle. Elle s'appuie sur :

- GCC ou Clang
- CMake + Ninja
- Qt6 (base + charts)
- libssh2
- zlib
- nlohmann-json

Installation des dépendances (exemple Debian/Ubuntu) puis compilation :

```bash
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-charts-dev qt6-base-dev-tools \
  libssh2-1-dev zlib1g-dev nlohmann-json3-dev

cmake --preset linux
cmake --build --preset linux
./build/SiteWatch
```

Pour intégrer SiteWatch au bureau (icône + raccourci) ou produire une AppImage
autonome, le guide pas à pas couvre chaque distribution :
**[docs/INSTALL_LINUX.md](docs/INSTALL_LINUX.md)**.

---

# Architecture

SiteWatch est organisé autour d'un moteur d'analyse indépendant de l'interface graphique.

Cette séparation permet de faire évoluer le logiciel sans impacter les différents composants et a facilité le portage Linux, désormais disponible.

L'architecture repose sur plusieurs couches :

```
+------------------------------------------------------+
|                    Interface Qt                      |
+------------------------------------------------------+
|        Présentation des analyses et graphiques       |
+------------------------------------------------------+
|           Moteur d'analyse des journaux              |
+------------------------------------------------------+
|     Téléchargement SFTP / Décompression / Cache      |
+------------------------------------------------------+
|           Apache / LiteSpeed Access Logs             |
+------------------------------------------------------+
```

Le moteur d'analyse ne dépend pas directement de l'interface graphique.

Cette organisation facilite :

- les évolutions fonctionnelles ;
- les tests ;
- le support multiplateforme (Windows et Linux) ;
- une éventuelle interface CLI ou service en arrière-plan.

---

# Dépendances principales

SiteWatch utilise principalement :

- Qt 6
- C++17
- CMake
- libssh2
- zlib
- nlohmann-json

Sous Windows, la chaîne de compilation officielle repose sur :

- MSYS2
- MinGW-w64
- GCC
- Ninja

---

# Philosophie

SiteWatch n'est pas un logiciel de statistiques.

Il ne cherche pas à remplacer Google Analytics, Matomo ou AWStats.

Son objectif est d'aider à comprendre ce qui s'est réellement passé sur un serveur web.

Pour cela, il privilégie :

- une lecture directe des journaux ;
- des analyses orientées administration système ;
- une navigation rapide grâce aux doubles-clics ;
- des vues synthétiques permettant d'identifier immédiatement les anomalies ;
- une approche multi-sites pensée pour les administrateurs.

Le logiciel est développé à partir de besoins réels rencontrés lors de l'administration quotidienne de plusieurs sites WordPress.

Chaque fonctionnalité répond à un problème concret rencontré sur le terrain.

---

# Documentation

Le dépôt contient une documentation complète destinée aussi bien aux utilisateurs qu'aux développeurs.

| Document | Description |
|----------|-------------|
| `README.md` | Présentation générale du projet |
| `GUIDE.md` | Guide utilisateur |
| `CHANGELOG.md` | Historique des versions |
| `ROADMAP.md` | Évolutions prévues |
| `CONTRIBUTING.md` | Guide de contribution |
| `docs/INSTALL_LINUX.md` | Installer et lancer sous Linux (AppImage, compilation, intégration) |
| `docs/BUILD_FOR_BEGINNERS.md` | Compilation simplifiée sous Windows |
| `docs/CASE_STUDIES.md` | Cas pratiques et investigations réelles |

La documentation est mise à jour en parallèle du développement afin de rester cohérente avec les fonctionnalités disponibles.

---

# Feuille de route

Les évolutions actuellement envisagées comprennent notamment :

- amélioration continue de l'analyse des robots ;
- enrichissement des analyses WordPress ;
- supervision multi-sites avancée ;
- amélioration des graphiques ;
- nouveaux rapports d'investigation ;
- préparation d'une architecture compatible avec une supervision continue.

Les fonctionnalités prévues sont détaillées dans `ROADMAP.md`.

---

# Contribuer

Les rapports de bugs, suggestions d'amélioration et contributions sont les bienvenus.

Avant de proposer une Pull Request, merci de consulter :

```
CONTRIBUTING.md
```

Pour signaler un problème ou proposer une évolution, utilisez les **Issues GitHub**.

---

# Licence

SiteWatch est distribué sous licence **GNU GPL v3.0 only**.

Toute redistribution ou modification doit respecter les termes de cette licence.

Voir le fichier :

```
LICENSE
```

pour le texte complet.

---

# Auteur

**morfredus**

Développeur, photographe et créateur d'outils open source.

La plupart de mes projets naissent d'un besoin concret rencontré sur le terrain. Lorsqu'une solution adaptée n'existe pas, je préfère construire la mienne puis la documenter afin qu'elle puisse être utile à d'autres.

---

# Remerciements

Merci à toutes les personnes qui prennent le temps :

- d'utiliser SiteWatch ;
- de signaler des bugs ;
- de proposer des améliorations ;
- de contribuer au projet.

Chaque retour permet de faire évoluer le logiciel dans la bonne direction.

---

## Projet Open Source

© 2026 **morfredus**

Distribué sous licence **GNU GPL v3.0 only**.