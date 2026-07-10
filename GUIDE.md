# Guide utilisateur — SiteWatch

Ce guide explique la configuration et l'utilisation quotidienne de SiteWatch.
Pour l'installation/compilation, voir le [README](README.md).
Sous **Linux**, suivre le guide dédié
[Installer et lancer SiteWatch sous Linux](docs/INSTALL_LINUX.md).
Sous **Windows**, pour débuter, suivre le guide
[Compiler SiteWatch quand on débute](docs/BUILD_FOR_BEGINNERS.md).
Pour les évolutions envisagées, consulter la [roadmap](ROADMAP.md).

---

## 1. Premier démarrage

Au tout premier lancement, aucun site n'est configuré. La barre de statut invite
à ouvrir **Fichier → Configuration…**.

SiteWatch range ses données dans l'emplacement standard du système :

| Élément | Windows | Linux |
|---|---|---|
| Configuration (`config.json`) | `%LOCALAPPDATA%\SiteWatch` | `~/.config/SiteWatch` |
| Cache des logs (par défaut) | `%LOCALAPPDATA%\SiteWatch\cache` | `~/.local/share/SiteWatch/cache` |

Le mot de passe et le jeton d'API ne sont stockés **que** dans ce fichier local
(jamais synchronisé, jamais versionné).

---

## 2. Configuration d'un site

Ouvrir **Fichier → Configuration…** (raccourci `Ctrl+,`).

### Emplacement des données

En haut, le champ **Cache des données** définit où sont stockés les logs
téléchargés. Bouton **Parcourir…** pour choisir, **Par défaut** pour revenir à
l'emplacement recommandé.

### Ajouter un site

Cliquer **+ Ajouter**, puis renseigner :

| Champ | Description |
|---|---|
| **Nom du site** | Le domaine **avec son extension**, ex. `monsite.fr`. Sert d'identifiant **et** à déduire automatiquement les fichiers de logs à télécharger. |
| **Serveur SFTP** | L'hôte SSH, ex. `serveur.o2switch.net`. |
| **Utilisateur** | Le compte SSH/cPanel. |
| **Clé SSH** | Chemin de la **clé privée** (bouton *Parcourir…*). Recommandé. |
| **Mot de passe** | Optionnel — utilisé en secours si la clé échoue (icône 👁 pour afficher). |
| **Dossier distant des logs** | Ex. `/home2/utilisateur/logs`. |
| **Jeton d'API cPanel** | Pour l'ouverture automatique du pare-feu o2switch (icône 👁). Laisser **vide** pour un autre hébergeur. |
| **Filtre des logs (avancé)** | Laisser **vide** pour o2switch. Voir §6 pour un autre hébergeur. |

L'indicateur devant chaque site : 🟠 à tester · 🟢 configuration valide · 🔴 erreur.

### Tester la connexion

Le bouton **Tester la connexion** valide immédiatement la configuration :

```
✓ Connexion SSH
✓ Lecture du dossier (92 fichiers)
✓ Préfixe détecté : monsitefr
✓ 4 fichier(s) pour ce site
Dernière connexion : 06/07/2026 22:10
```

En cas d'échec, le message indique l'étape fautive (pare-feu, connexion, auth).

Cliquer **Enregistrer** pour valider.

---

## 3. Authentification SSH (o2switch)

o2switch exige une **clé SSH autorisée** et l'**ouverture du pare-feu**.

1. **Clé SSH** — dans le cPanel : *Sécurité → Accès SSH → Gérer les clés SSH*.
   Générer ou importer une clé, **l'autoriser**, télécharger la **clé privée**,
   puis indiquer son chemin dans le champ *Clé SSH*. La clé publique `.pub` est déduite
   automatiquement si elle est absente.
2. **Jeton d'API cPanel** — dans le cPanel : *Sécurité → Gérer les jetons d'API*.
   Créer un jeton et le coller dans le champ *Jeton d'API cPanel*. SiteWatch
   s'en sert pour autoriser l'IP publique locale (souvent dynamique) avant
   chaque synchro.

> Sans jeton, le port SSH reste fermé par le pare-feu o2switch et la connexion
> échoue (« Connexion TCP impossible »).

---

## 4. Utilisation quotidienne

### Synchroniser / Analyser

- **Synchroniser** : ouvre le pare-feu, télécharge les nouveaux logs (barre de
  progression), puis analyse. Les fichiers déjà présents et inchangés ne sont
  jamais retéléchargés.
- **Analyser** : (re)analyse les logs déjà en cache, sans connexion réseau.

### Période

En haut à droite : deux dates + un menu de raccourcis (Aujourd'hui, 7/30 jours,
ce mois, mois dernier, cette/année dernière, personnalisé). **Tous les onglets
se recalculent** selon la période choisie.

### Les onglets

| Onglet | Contenu |
|---|---|
| **Sites** | Vue globale de tous les sites : état, priorité, points d'attention, action recommandée et synthèse. **Double-cliquer** un site pour ouvrir son analyse détaillée. |
| **Santé** | Verdict global 🟢/🟠/🔴 + indicateurs. **Double-cliquer** un indicateur pour sauter à l'onglet concerné (avec le bon filtre). |
| **Robots** | Robots par catégorie (IA / moteurs / SEO / divers) avec %, donut de répartition et top robots. |
| **Sécurité** | Tentatives d'attaque + erreurs 404/403/500, avec pastilles 🔴/🟠. |
| **Activité WP** | Activité WordPress légitime (admin-ajax, REST, wp-login, wp-admin) avec %. |
| **Top pages** | Vraies pages consultées (assets techniques filtrés). |
| **Référents** | Provenance (Direct, Google, Bing, DuckDuckGo, réseaux sociaux…). |
| **URLs** | Menu **Affichage** : toutes / attaques probables / fonctionnement WordPress / erreurs 404 / requêtes système. |
| **Graphiques** | Évolution (trafic, robots IA, robots SEO, 404, activité WP, attaques). |
| **Recherche** | Recherche par IP, URL, robot, date (`2026-07-06`) ou code HTTP. |

### Détail au double-clic (tous les onglets)

**Double-cliquer** n'importe quelle ligne des onglets **Sécurité**, **Activité
WP**, **Top pages**, **Référents**, **URLs** ou **Recherche** : une fenêtre
affiche le détail des entrées correspondantes — IP, User-Agents, codes HTTP,
URLs, référents, répartition horaire et évolution par jour. Le **site concerné**
est rappelé en haut de la fenêtre.

Selon l'onglet, le détail porte sur : l'URL (Top pages, URLs), la catégorie
d'attaque ou l'erreur (Sécurité), l'activité WordPress (Activité WP), la
provenance (Référents), ou l'URL du résultat (Recherche).

Pour un exemple réel d'investigation, lire les
[études de cas](docs/CASE_STUDIES.md). Le premier cas montre comment des 404
peu visibles dans les logs bruts ont permis d'identifier une extension WordPress
devenue inutile.

### Copier / exporter

**Clic droit** sur une ou plusieurs lignes sélectionnées (Ctrl+clic) de n'importe
quel onglet : **Copier les infos** (presse-papier) ou **Exporter en CSV…**
(format international). Dans la fenêtre de détail, le clic droit sur un tableau
copie/exporte la sélection, et les boutons **Copier tout** / **Exporter tout**
reprennent l'ensemble.

### Apparence (thème clair / sombre)

Le menu **Affichage → Thème** propose trois choix :

| Choix | Effet |
|---|---|
| **Système** *(par défaut)* | Suit automatiquement l'apparence claire ou sombre du système d'exploitation, et s'adapte si celle-ci change. |
| **Clair** | Force le thème clair, quel que soit le réglage du système. |
| **Sombre** | Force le thème sombre, quel que soit le réglage du système. |

Le choix est mémorisé et réappliqué au prochain démarrage.

---

## 5. Outils

- **Onglet Sites** : affiche la supervision globale de tous les sites sur la
  période courante. Double-cliquer un site pour ouvrir son analyse détaillée.
- **Outils → Effacer des logs téléchargés…** : supprime des `.gz` du cache. Filtre
  par site (ou tous), par étendue (tout / antérieurs à un mois / période), avec
  une liste cochable pour affiner. Confirmation avant suppression.

---

## 6. Adapter à un autre hébergeur

SiteWatch cible o2switch mais reste ouvert :

1. **Pare-feu** : laisser le champ *Jeton d'API cPanel* **vide** → connexion SSH
   directe, sans étape d'autorisation.
2. **Nommage des logs** : si l'hébergeur nomme ses fichiers autrement, renseigner
   le champ **Filtre des logs (avancé)** avec un motif que le nom de fichier doit
   **contenir** (ex. `monsite.fr` ou `access`). Cela remplace la détection
   automatique o2switch.

---

## 7. Aide

- **Aide → Aide** (`F1`) : rappel de la prise en main.
- **Aide → À propos** : version et crédits.

---

## Licence

Copyright (C) 2026 morfredus

Ce projet est distribué sous les termes de la **GNU General Public License v3.0**.
Voir le fichier [`LICENSE`](LICENSE) pour le texte complet.
