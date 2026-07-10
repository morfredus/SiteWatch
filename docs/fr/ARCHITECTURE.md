# Architecture et philosophie

Retour à l'[index de la documentation](README.md).

---

## Architecture

SiteWatch est organisé autour d'un moteur d'analyse indépendant de l'interface
graphique.

Cette séparation permet de faire évoluer le logiciel sans impacter les différents
composants et a facilité le portage Linux, désormais disponible.

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

## Dépendances principales

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

Pour compiler le projet, voir [Compiler depuis les sources](COMPILATION.md).

---

## Philosophie

SiteWatch n'est pas un logiciel de statistiques.

Il ne cherche pas à remplacer Google Analytics, Matomo ou AWStats.

Son objectif est d'aider à comprendre ce qui s'est réellement passé sur un
serveur web.

Pour cela, il privilégie :

- une lecture directe des journaux ;
- des analyses orientées administration système ;
- une navigation rapide grâce aux doubles-clics ;
- des vues synthétiques permettant d'identifier immédiatement les anomalies ;
- une approche multi-sites pensée pour les administrateurs.

Le logiciel est développé à partir de besoins réels rencontrés lors de
l'administration quotidienne de plusieurs sites WordPress.

Chaque fonctionnalité répond à un problème concret rencontré sur le terrain.
