# Dépannage — « Je n'arrive pas à télécharger mes logs »

Ce guide explique, en langage simple, les messages que SiteWatch affiche quand
vous cliquez sur **Télécharger les logs**, et comment régler chaque situation.
Aucune compétence technique n'est nécessaire.

> Nouveau depuis la version 1.4.0 : SiteWatch **explique** le problème dans un
> bandeau coloré et, quand c'est possible, **propose la solution en un clic**.

---

## 1. Comment ça marche, en deux phrases

Quand vous cliquez sur **Télécharger les logs**, SiteWatch se connecte à votre
hébergement en SSH/SFTP, regarde les fichiers de logs présents, garde ceux de
**votre** site, et les télécharge. Selon ce qu'il trouve, il affiche un
**bandeau** juste sous la barre d'outils :

| Couleur | Ce que ça veut dire |
|---|---|
| 🟢 **Vert** | Tout va bien : les logs ont été téléchargés (ou étaient déjà à jour). |
| 🟠 **Orange** | Attention : il n'a rien trouvé à télécharger pour ce site. |
| 🔴 **Rouge** | Erreur : il n'a pas pu se connecter ou lire le dossier. |

Le bandeau ne bloque pas l'application : vous pouvez le lire, cliquer sur son
bouton s'il y en a un, ou le fermer avec la croix **✕**.

---

## 2. Les messages 🔴 rouges (erreurs)

### « Connexion SFTP impossible »

SiteWatch n'a pas réussi à se connecter au serveur.

- Vérifiez le **Serveur SFTP** (l'hôte), l'**Utilisateur**, et le **mot de passe**
  ou la **clé SSH** dans **Fichier → Configuration…**.
- Le bouton **Tester la connexion** de la configuration vous dit précisément
  quelle étape échoue.

### « Pare-feu o2switch : autorisation refusée »

Chez o2switch, le port SSH est fermé tant qu'il n'est pas autorisé.

- Vérifiez le **Jeton d'API cPanel** dans la configuration du site.
- Si vous n'êtes **pas** chez o2switch, laissez ce champ **vide**.

### « Dossier distant illisible »

Le chemin des logs sur le serveur est probablement incorrect.

- Vérifiez le champ **Dossier distant des logs** (ex. `/home2/moncompte/logs`).

---

## 3. Les messages 🟠 orange (rien à télécharger)

### « Aucun log (.gz) dans le dossier distant »

Le dossier est bien lisible, mais il ne contient aucun fichier de logs compressé.

- Vérifiez que le **Dossier distant des logs** est le bon.
- Sinon, patientez : beaucoup d'hébergeurs ne compressent les logs de la veille
  que le lendemain. Les fichiers `.gz` apparaîtront plus tard.

### « X fichiers présents, mais aucun ne correspond au filtre »

C'est le cas le plus courant. Des logs existent bien, mais leur **nom** ne
correspond pas à ce que SiteWatch cherche pour votre site. Selon l'hébergeur, les
fichiers peuvent s'appeler très différemment :

```
monsitefr.ssl.log-20260710.gz
monsite.fr.ssl.log-20260710.gz
client123.ssl.log-20260710.gz
```

**Bonne nouvelle :** SiteWatch lit les noms présents et, quand il reconnaît un
préfixe commun fiable, il **propose directement le bon filtre** avec un bouton :

> ⚠️ 2 fichiers présents sur le serveur, mais aucun ne correspond au filtre actuel.
> SiteWatch a détecté un préfixe commun : **tabacclaouey.fr** (2 fichiers).
> **[ Utiliser ce filtre : tabacclaouey.fr ]**

Cliquez sur le bouton : le filtre est **enregistré** et le téléchargement
**redémarre tout seul**. En général, c'est terminé.

Si aucun bouton n'apparaît (préfixes trop variés pour décider sans risque),
réglez le filtre à la main — voir la section suivante.

---

## 4. Régler le filtre à la main

1. Ouvrez **Fichier → Configuration…**.
2. Dans le champ **Filtre des logs (avancé)**, tapez un morceau de texte que le
   nom des fichiers **contient** — le plus souvent votre domaine :

   | Vos fichiers ressemblent à… | Filtre à saisir |
   |---|---|
   | `monsite.fr.ssl.log-20260710.gz` | `monsite.fr` |
   | `monsitefr.ssl.log-20260710.gz` | `monsitefr` |
   | `client123.ssl.log-20260710.gz` | `client123` |

3. **Enregistrez**, puis relancez **Télécharger les logs**.

> Astuce : le bouton **Tester la connexion** (dans la configuration) affiche le
> **préfixe détecté** et le nombre de fichiers correspondants — pratique pour
> vérifier votre filtre avant de fermer la fenêtre.

Pour o2switch avec un nom de site standard, laissez ce champ **vide** : la
détection est automatique.

---

## 5. Toujours bloqué ?

- Relisez le [Guide utilisateur](GUIDE.md), section « Télécharger les logs ».
- Sous Linux, vérifiez l'[installation](INSTALL_LINUX.md).
- Vous pouvez ouvrir une question sur le dépôt du projet (voir le
  [README](../../README.md)).

---

## Licence

Copyright (C) 2026 morfredus — distribué sous **GNU GPL v3.0**
(voir [`LICENSE`](../../LICENSE)).
