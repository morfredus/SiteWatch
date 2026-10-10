# Publication sans interface (`sitewatch-publish`)

Retour à l'[index de la documentation](README.md).

---

SiteWatch envoie ses synthèses à morfAnalytics quand l'interface est ouverte. Si la
fenêtre reste fermée plusieurs jours, la page `/sitewatch` de morfAnalytics reste figée
sur le dernier rapport. `sitewatch-publish` fait la même chose sans interface, pour être
planifié.

## Ce qu'il fait

1. **Collecteur** (si un morfCollector répond) : aligne la configuration, puis recopie
   dans le cache les logs récoltés pendant l'absence. Seuls les fichiers manquants ou de
   taille différente sont transférés.
2. **Vérité web** : consolide, pour chaque site, les séries quotidiennes de tout
   l'historique en cache (`web/web.sqlite` sous la racine du cache).
3. **Publication** : envoie chaque site à morfAnalytics (`POST /sitewatch/ingest`), puis
   rattrape et publie GitHub (`POST /github/ingest`).

Il utilise le même code et les mêmes magasins que l'interface : ouvrir SiteWatch ensuite
n'a rien à refaire. Sans collecteur ni morfAnalytics, il consolide simplement le cache
local (SiteWatch reste autonome).

## Utilisation

```sh
sitewatch-publish [config.json] [--collector URL] [--analytics URL] [--days N] [--no-collector]
```

| Option | Rôle |
|---|---|
| `config.json` | Fichier de configuration. Par défaut, celui de l'interface. |
| `--collector URL` | Adresse de morfCollector (sinon `collectorUrl`, sinon découverte morfBeacon). |
| `--analytics URL` | Adresse de morfAnalytics (sinon `analyticsUrl`). Sans adresse, rien n'est publié. |
| `--days N` | Période de la synthèse publiée (60 jours par défaut). |
| `--no-collector` | Ne contacte aucun collecteur. |

Code de retour : `0` tout va bien, `1` au moins une étape a échoué, `2` configuration
illisible.

## Planifier

Windows (tâche planifiée quotidienne à 03:00, après la collecte de 02:00) :

```bat
schtasks /Create /SC DAILY /ST 03:00 /TN "SiteWatch publication" ^
  /TR "\"C:\Chemin\SiteWatch\sitewatch-publish.exe\""
```

Linux (timer systemd) : une unité `oneshot` qui lance la commande, et un timer
`OnCalendar=*-*-* 03:00:00` avec `Persistent=true`, afin de rattraper un démarrage manqué.

Renseigner `collectorUrl` et `analyticsUrl` avec un **nom d'hôte** (par exemple
`http://pi4dev:8792`) plutôt qu'une adresse IP : une IP attribuée en DHCP change.
