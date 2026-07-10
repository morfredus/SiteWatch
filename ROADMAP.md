# Roadmap — SiteWatch

Cette roadmap décrit les évolutions possibles de SiteWatch. Elle n'est pas une
promesse de date : elle sert à garder une direction claire pour les prochaines
versions.

## Vision

SiteWatch doit aider à décider rapidement où intervenir.

L'objectif n'est pas d'ajouter toujours plus de statistiques, mais de rendre les
signaux importants immédiatement visibles : erreurs serveur, attaques, robots,
activité WordPress inhabituelle, comportement de Google et problèmes d'URLs.

## Court terme

- Améliorer l'onglet **Sites** comme tableau de bord principal après une
  synchronisation.
- Ajouter une synchronisation globale explicite pour tous les sites configurés.
- Rendre les points d'attention plus précis sans introduire d'IA : règles
  simples, compréhensibles et vérifiables.
- Ajouter davantage de messages d'aide dans l'interface quand aucun log ou aucune
  donnée n'est disponible.

## Moyen terme

- Comparer une période avec la période précédente, par exemple les 30 derniers
  jours contre les 30 jours précédents.
- Afficher les variations importantes : humains, robots IA, attaques, erreurs
  404, erreurs 500, activité Google.
- Afficher un petit historique de l'état d'un site sur les dernières
  synchronisations.
- Conserver les synthèses de santé pour suivre l'évolution d'un site dans le
  temps.

## Alertes possibles

- Nouveau site en état rouge.
- Forte hausse des attaques.
- Explosion des erreurs 404.
- Baisse importante de l'activité Google.
- Hausse inhabituelle des robots IA.
- Activité WordPress anormale.

## Exploitation et maintenance

- Mieux documenter les cas d'hébergeurs non-o2switch.
- Faciliter l'export des données utiles pour un diagnostic.
- Continuer à simplifier la compilation Windows autour de MSYS2/MinGW.
- Maintenir le déploiement Linux (AppImage + intégration au bureau) et le tenir
  à jour avec les nouvelles versions de Qt.
- Garder une architecture où les règles de santé sont centralisées et
  réutilisables par l'onglet Santé, l'onglet Sites et les futures alertes.

## Hors objectif

SiteWatch n'a pas vocation à remplacer un outil marketing de mesure d'audience.
La priorité reste l'administration, la supervision technique et la compréhension
des événements réellement présents dans les logs serveur.
