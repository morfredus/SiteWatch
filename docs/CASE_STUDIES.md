# SiteWatch - Case Studies

Ce document regroupe des cas réels rencontrés lors de l'utilisation de SiteWatch.

L'objectif n'est pas de démontrer les fonctionnalités de l'application, mais de montrer comment une simple analyse des journaux d'accès peut conduire à la découverte de problèmes techniques qui seraient probablement restés invisibles.

---

# Case #001 - Découverte d'un conflit de gestion des polices WordPress

## Contexte

Site WordPress 7.0 utilisant le thème Blocksy.

Le site fonctionnait parfaitement :
- aucun problème d'affichage visible ;
- aucun signalement utilisateur ;
- excellent score PageSpeed.

Aucun symptôme ne laissait penser à un problème.

---

## Détection

SiteWatch a mis en évidence un nombre inhabituel de requêtes sur plusieurs fichiers de polices :

```
/wp-content/uploads/fonts/*.woff2
```

Les statistiques montraient principalement :

- HTTP 404
- quelques HTTP 302

Il ne s'agissait pas d'une attaque mais d'une ressource réellement demandée par les visiteurs.

---

## Investigation

L'analyse a suivi plusieurs étapes.

### Vérification des fichiers

Les fichiers référencés dans les journaux n'existaient plus.

### Recherche dans la base WordPress

Les recherches ont montré que les anciennes références existaient toujours dans :

- wp_font_face
- wp_global_styles

### Vérification du code HTML

Le code source des pages contenait :

```html
<style class="wp-fonts-local">
```

WordPress injectait toujours des règles `@font-face` pointant vers les anciens fichiers.

### Analyse des plugins

Le plugin **Local Google Fonts** était également installé.

Ce plugin générait une seconde copie des polices dans un autre emplacement avec une structure différente.

Deux systèmes géraient donc simultanément les Google Fonts :

- WordPress (Font Library)
- Local Google Fonts

---

## Cause

L'installation utilisait simultanément :

- la bibliothèque de polices native de WordPress ;
- le plugin Local Google Fonts.

Les deux mécanismes n'utilisaient pas la même organisation des fichiers.

WordPress continuait à générer des URLs vers d'anciens fichiers devenus inexistants.

---

## Résolution

Le plugin Local Google Fonts a été supprimé.

Les polices sont désormais entièrement gérées par la bibliothèque native de WordPress.

Résultats :

- disparition des erreurs 404 sur les polices ;
- architecture simplifiée ;
- un plugin de moins à maintenir ;
- aucun changement visible pour les visiteurs.

---

## Ce que SiteWatch a apporté

Sans SiteWatch, ce problème serait probablement passé totalement inaperçu.

Le site fonctionnait.

Les visiteurs ne voyaient aucune erreur.

Les moteurs de performance ne signalaient rien de critique.

Ce sont uniquement les journaux d'accès qui ont permis d'identifier cette anomalie.

---

## Enseignement

Toutes les erreurs ne correspondent pas à une attaque ou à un dysfonctionnement visible.

Certaines révèlent simplement une dette technique.

En analysant régulièrement les journaux d'accès, il est possible de découvrir :

- des ressources orphelines ;
- des plugins devenus inutiles ;
- des conflits entre extensions ;
- des références obsolètes ;
- des optimisations possibles.

Ce cas illustre la philosophie de SiteWatch :

> Les journaux racontent souvent bien plus que des visites ou des erreurs.

Ils permettent de comprendre ce qui se passe réellement sur un site et d'améliorer progressivement sa qualité technique.