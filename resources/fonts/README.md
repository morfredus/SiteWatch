# SiteWatch Icons (police d'icônes de l'interface)

`SiteWatchIcons.ttf` est un **sous-ensemble** de **Font Awesome 6 Free (Solid)**,
réduit aux seuls glyphes utilisés par l'interface de SiteWatch (≈ 4 Ko).

Elle est chargée au démarrage (voir `src/ui/Icons.cpp`) et sert à afficher les
pictogrammes de l'UI (cartes KPI, bannière, pastilles d'état, etc.) de façon
**identique sur Windows, Linux et WSL**, sans dépendre des polices emoji du
système.

## Licence et attribution

- Icônes : **Font Awesome Free**, © Fonticons, Inc. — <https://fontawesome.com>
- La police est distribuée sous **SIL OFL 1.1** (voir `LICENSE-FontAwesome.txt`).
- Conformément à l'OFL, cette version modifiée (sous-ensemble) a été **renommée**
  « SiteWatch Icons » et n'utilise pas le nom de police réservé d'origine.

## Régénérer le sous-ensemble

Avec `fonttools` (pyftsubset) à partir de `fa-solid-900.ttf`, en conservant les
codepoints listés dans `src/ui/Icons.cpp`, puis en renommant la famille en
« SiteWatch Icons ».
