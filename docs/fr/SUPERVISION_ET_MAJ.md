# Supervision réseau et mises à jour

Retour à l'[index de la documentation](README.md).

---

SiteWatch embarque deux petits **modules communs** (préfixe « morf »), partagés
avec les autres applications (ComponentHub, et les outils à venir) :

- **morfBeacon** — *supervision réseau* : l'application annonce sa présence sur le
  réseau local et expose ses métriques, pour être suivie depuis un tableau de bord
  central (le **morfDashboard**).
- **morfUpdate** — *mises à jour* : vérification de la dernière version publiée sur
  GitHub.

> À ne pas confondre : SiteWatch **supervise des sites web** (via leurs logs) ;
> morfBeacon permet de **superviser l'application SiteWatch elle-même** (est-elle
> lancée, sur quelle machine, depuis combien de temps, avec quelles métriques).

Ces modules sont **vendorés** dans le projet (dossier `third_party/morf/`) : ils se
compilent avec l'application, sans dépendance externe, et se comportent donc à
l'identique sous Windows, Linux et Raspberry Pi.

---

## morfBeacon — « je suis actif » + métriques

### Le principe : annoncer, plutôt qu'être scanné

Au lieu qu'un superviseur interroge en boucle toutes les machines du réseau,
chaque application **annonce spontanément** son existence. Deux canaux, deux rôles
bien distincts :

| Besoin | Canal | Sens |
|---|---|---|
| « L'appli est-elle en vie ? » | **Heartbeat UDP** en broadcast (port `45454`) | l'appli **émet** toutes les 15 s |
| « Quel est son état détaillé ? » | Serveur **HTTP `/status`** (port `8788`) | le superviseur **interroge** à la demande |

Le superviseur se contente d'**écouter** le heartbeat : aucune configuration,
aucune IP à connaître, découverte automatique. Une application qui n'émet plus
pendant environ une minute est considérée hors ligne. Les métriques détaillées ne
voyagent **jamais** dans le heartbeat (qui reste minuscule) : elles restent
derrière `/status`, interrogé seulement quand c'est utile.

### Ce que SiteWatch expose

Heartbeat UDP (JSON compact, broadcast sur le port `45454`) :

```json
{
  "proto": "morfbeacon/1", "app": "SiteWatch", "host": "supervision-pi",
  "version": "1.4.2", "state": "ok", "status_port": 8788,
  "instance": "SiteWatch@supervision-pi", "uptime_s": 3600, "ts": 1752400000
}
```

`GET http://<machine>:8788/status` :

```json
{
  "app": "SiteWatch", "version": "1.4.2", "state": "ok", "uptime_s": 3600,
  "metrics": {
    "sites_configured": 3, "last_total_requests": 128470,
    "last_humans": 4210, "last_bots": 9033, "last_404": 512,
    "last_analysis_date": "2026-07-13"
  },
  "ts": 1752400000
}
```

Les champs `last_*` reflètent la **dernière analyse** effectuée (ils valent 0 tant
qu'aucune analyse n'a été lancée dans la session). `GET /healthz` renvoie
`{"status":"ok"}` (sonde de vie légère).

### Le tester

L'application lancée, dans un terminal :

```sh
curl http://localhost:8788/status
```

Ou avec le testeur fourni dans le dépôt morfBeacon (écoute le heartbeat +
interroge `/status`, sans aucune dépendance) :

```sh
python3 morfBeacon/tools/fake_dashboard.py --poll
```

Il affiche `DECOUVERT SiteWatch@…` en quelques secondes, puis les métriques.
C'est exactement ce que fera le morfDashboard.

### Les ports

- **UDP `45454`** : commun à **toutes** les applications (canal d'annonce partagé).
- **HTTP `8788`** : propre à SiteWatch. (ComponentHub utilise `8787`, GatewayLab `8789`.)

---

## morfUpdate — vérification des mises à jour

SiteWatch compare sa version (fichier `VERSION`) à la dernière *release* publiée
sur GitHub (`morfredus/SiteWatch`) :

- **Au démarrage**, une vérification **silencieuse** : une notification n'apparaît
  que si une version plus récente existe.
- **À la demande**, via le menu **Aide → « Rechercher les mises à jour… »** : là,
  un message s'affiche aussi si vous êtes déjà à jour, ou en cas d'erreur réseau.

La notification propose d'ouvrir la page de la *release* (ou le binaire) dans le
navigateur. **Aucune installation automatique** : le téléchargement reste à votre
main — un choix de prudence.

---

## Les modules vendorés (`third_party/morf/`)

```
third_party/morf/
├── beacon/   morfBeacon (supervision)
└── update/   morfUpdate (mises à jour)
```

Le code y est une **copie** des bibliothèques communes, compilée **statiquement**
dans l'exécutable. Conséquences :

- **Compilation sans dépendance externe** : aucun dépôt voisin n'est requis pour
  construire SiteWatch.
- **Rien à ajouter dans les paquets** (ZIP Windows, `.deb`, AppImage) : tout est
  dans le binaire.
- **Identique sur les trois plateformes** : même code, compilé avec le même Qt que
  l'application (seule dépendance : `Qt6::Network`, déjà utilisé par SiteWatch).

### Mettre à jour ces modules

Ne modifiez pas le code sous `third_party/` : la **source de vérité** est dans les
dépôts voisins `morfBeacon` / `morfUpdate`. Pour resynchroniser la
copie après une évolution :

```sh
# Windows
powershell -ExecutionPolicy Bypass -File scripts\sync-morf.ps1
# Linux / Raspberry Pi
scripts/sync-morf.sh
```

Le script recopie `include/`, `src/` et `VERSION` sans toucher au `CMakeLists.txt`
vendoré. Détails dans [`third_party/morf/README.md`](../../third_party/morf/README.md).
