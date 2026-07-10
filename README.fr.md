# SiteWatch

[![Version](https://img.shields.io/badge/version-1.4.2-blue)](CHANGELOG.md)
[![GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20Raspberry%20Pi-lightgrey)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake)
![Status](https://img.shields.io/badge/Status-Active-success)

**🌍 Langue :** [English](README.md) · Français

> ✅ **Compilation et fonctionnement vérifiés** sous **Windows 11**
> (MSYS2/MinGW, Qt 6.11), **Linux Mint 22.3 « Zena »** (base Ubuntu 24.04 LTS,
> Qt 6.4) et **Raspberry Pi 4** (Raspberry Pi OS 64 bits).

**Outil de bureau multiplateforme d'investigation des journaux d'accès Apache et LiteSpeed.**

SiteWatch est une **application Qt / C++17 multiplateforme** dédiée à
l'administration et à la supervision de sites web. Elle s'exécute nativement sous
**Windows, Linux (x86_64 et ARM64) et Raspberry Pi** — Windows n'est qu'une
plateforme supportée parmi d'autres, pas la cible unique.

Contrairement aux outils de statistiques classiques, elle ne cherche pas à
compter les visiteurs ni à produire des rapports marketing. Son objectif est
beaucoup plus simple :

> **Comprendre ce qui s'est réellement passé sur un serveur web.**

Il télécharge les journaux Apache ou LiteSpeed via SFTP, les analyse localement
et présente les résultats sous une forme immédiatement exploitable — **aucune
base SQL, aucune dépendance WordPress, aucun plugin à installer sur les sites
surveillés**. Il lit directement les fichiers de logs compressés (`.gz`).

SiteWatch fonctionne avec tout hébergeur proposant un accès SSH/SFTP et dispose
d'une intégration avancée pour **o2switch** (ouverture automatique du pare-feu
via l'API cPanel).

N'étant plus lié à Windows, SiteWatch convient aux installations sobres et
toujours allumées :

- un **Raspberry Pi** qui surveille plusieurs sites en permanence ;
- une **VM Linux** exécutant SiteWatch en tâche planifiée ;
- un **NAS Debian** ou un **mini-PC fanless** consommant quelques watts.

## Pourquoi SiteWatch ?

Les outils classiques répondent à des questions comme *« combien ai-je eu de
visiteurs ? »* ou *« quelle est ma page la plus consultée ? »*. SiteWatch répond
à d'autres questions :

- Pourquoi cette URL retourne-t-elle autant de 404 ?
- Quel robot scanne actuellement mon site ?
- Pourquoi Google demande-t-il cette ressource ?
- Cette activité WordPress est-elle normale ?
- Est-ce une vraie attaque ou un faux positif ?
- Quel site nécessite mon intervention aujourd'hui ?

Il est conçu comme un **outil d'investigation** destiné aux administrateurs de
sites web.

---

## Captures

**Tableau de bord** — vue globale multi-sites après chaque synchronisation ou
analyse, pour repérer en quelques secondes les sites à surveiller. Un double-clic
sur un site ouvre son analyse détaillée.

![Tableau de bord](docs/pictures/dashboard.png)

**Santé** — les principaux indicateurs du site sélectionné, avec des états
🟢 🟠 🔴 qui mènent directement à l'onglet concerné.

![Santé](docs/pictures/health.png)

**Robots** — classement automatique par catégorie (IA, moteurs, SEO, divers)
avec graphique de répartition.

![Robots](docs/pictures/robots.png)

**Sécurité** — requêtes suspectes regroupées pour distinguer rapidement
l'activité normale des véritables tentatives d'attaque, faux positifs WordPress
écartés.

![Sécurité](docs/pictures/security.png)

**Activité WordPress** — activités WordPress légitimes (admin, connexion, REST
API, `admin-ajax.php`, XML-RPC, cron…) identifiées pour distinguer le
fonctionnement normal des comportements inhabituels.

![Activité WordPress](docs/pictures/wordpress-activity.png)

**Top pages & référents** — les pages les plus demandées et la provenance du
trafic (moteurs, réseaux sociaux, accès directs, autres sites).

![Top pages](docs/pictures/top-pages.png)
![Référents](docs/pictures/referrers.png)

**URLs & fenêtres de détail** — exploration de chaque URL par catégorie (toutes,
attaques probables, 404, WordPress, robots, système). Un double-clic ouvre une
fenêtre de détail complète : IP, User-Agents, codes HTTP, référents, répartition
horaire et quotidienne, avec copie et export CSV partout.

![URLs](docs/pictures/urls.png)
![Fenêtre de détail](docs/pictures/urls-detail.png)

**Graphiques** — évolution des principaux indicateurs sur la période analysée
(trafic humain, robots, robots IA, Google, erreurs HTTP, activité WordPress).

![Graphiques](docs/pictures/graph.png)

---

## Fonctionnalités

- Analyse locale des journaux Apache et LiteSpeed
- Téléchargement incrémental via SFTP (seuls les fichiers nouveaux ou modifiés)
- Gestion multi-sites et tableau de bord multi-sites
- Tableau de santé interactif
- Détection automatique des robots, y compris robots IA et moteurs de recherche
- Détection des attaques courantes et analyse de l'activité WordPress
- Analyse des URLs, des référents et des top pages
- Recherche avancée et fenêtres de détail interactives
- Copie dans le presse-papiers et export CSV
- Cache local avec nettoyage
- Configuration entièrement graphique
- Thèmes clair / sombre / système (suit automatiquement l'apparence de l'OS)
- Intégration de l'API o2switch pour l'ouverture automatique du pare-feu

---

## Installation

Aucun composant n'est installé sur les serveurs surveillés : SiteWatch fonctionne
uniquement à partir des journaux Apache/LiteSpeed téléchargés en SFTP.
L'application est portable.

### Windows

Extraire `SiteWatch-<version>-win64.zip` et exécuter `SiteWatch.exe`. Aucune
installation nécessaire.

### Linux

La solution la plus simple est l'**AppImage** : un fichier unique et autonome,
sans compilation ni dépendance à installer. Après téléchargement depuis les
releases :

```bash
chmod +x SiteWatch-<version>-x86_64.AppImage
./SiteWatch-<version>-x86_64.AppImage
```

Sous **Debian / Ubuntu / Raspberry Pi OS**, vous pouvez aussi produire un paquet
`.deb` (`scripts/linux/package-deb.sh`) qui s'installe proprement via apt.

Pour ajouter l'icône au menu des applications, ou pour compiler puis intégrer
SiteWatch au bureau, voir le guide dédié :
**[docs/fr/INSTALL_LINUX.md](docs/fr/INSTALL_LINUX.md)**.

Pour compiler depuis les sources (Windows et Linux), voir
**[docs/fr/COMPILATION.md](docs/fr/COMPILATION.md)**.

---

## Documentation

Toute la documentation utilisateur est regroupée (en français) dans
[`docs/fr/`](docs/fr/README.md). Un index anglais est en préparation dans
[`docs/en/`](docs/en/README.md).

- 📖 Guide utilisateur : [`docs/fr/GUIDE.md`](docs/fr/GUIDE.md)
- 🆘 Dépannage du téléchargement des logs : [`docs/fr/DEPANNAGE_LOGS.md`](docs/fr/DEPANNAGE_LOGS.md)
- 🐧 Installer et lancer sous Linux : [`docs/fr/INSTALL_LINUX.md`](docs/fr/INSTALL_LINUX.md)
- 🛠 Compiler depuis les sources : [`docs/fr/COMPILATION.md`](docs/fr/COMPILATION.md)
- 🧭 Architecture et philosophie : [`docs/fr/ARCHITECTURE.md`](docs/fr/ARCHITECTURE.md)
- 📚 Études de cas : [`docs/fr/CASE_STUDIES.md`](docs/fr/CASE_STUDIES.md)
- 🗺 Roadmap : [`ROADMAP.md`](ROADMAP.md)
- 🤝 Contribuer : [`CONTRIBUTING.md`](CONTRIBUTING.md)
- 📝 Journal des versions : [`CHANGELOG.md`](CHANGELOG.md)

---

## Contribuer

Les rapports de bugs, suggestions et contributions sont les bienvenus. Merci de
consulter [`CONTRIBUTING.md`](CONTRIBUTING.md) avant de proposer une Pull
Request, et d'utiliser les **Issues GitHub** pour signaler un problème ou
proposer une évolution.

## Licence

SiteWatch est distribué sous licence **GNU GPL v3.0 only**. Voir le fichier
[`LICENSE`](LICENSE) pour le texte complet.

## Auteur

**morfredus** — développeur, photographe et créateur d'outils open source.
La plupart de mes projets naissent d'un besoin concret rencontré sur le terrain.
Voir [`AUTHORS`](AUTHORS).

© 2026 morfredus — GNU GPL v3.0 only.
