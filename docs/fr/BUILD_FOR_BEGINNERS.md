# Compiler SiteWatch quand on débute

Ce guide explique comment compiler SiteWatch sur Windows avec VS Code, sans
connaissance préalable de CMake, Ninja ou Qt.

> **Sous Linux ?** Ce guide est spécifique à Windows. Pour installer, compiler ou
> intégrer SiteWatch sous Linux (AppImage prête à l'emploi comprise), suivre
> plutôt [Installer et lancer SiteWatch sous Linux](INSTALL_LINUX.md).

Le chemin recommandé est :

1. installer MSYS2 ;
2. installer les dépendances avec une seule commande ;
3. ouvrir le projet dans VS Code ;
4. lancer la compilation avec `Ctrl+Shift+B`.

## Pourquoi ce guide utilise MSYS2/MinGW

SiteWatch utilise une seule voie Windows officielle : **MSYS2/MinGW**.

Ce n'est pas une régression par rapport à MSVC/vcpkg. C'est une simplification
volontaire :

- une seule liste de dépendances à installer ;
- une seule configuration VS Code à maintenir ;
- moins de problèmes de chemins entre Qt, CMake, Ninja et les DLL ;
- le même fonctionnement en ligne de commande et depuis VS Code.

L'ancien fichier `vcpkg.json` n'est donc plus nécessaire.

## 1. Installer les outils

### Installer VS Code

Installer Visual Studio Code depuis :

<https://code.visualstudio.com/>

Ouvrir ensuite VS Code une première fois.

### Installer MSYS2

Installer MSYS2 depuis :

<https://www.msys2.org/>

Sur Windows, il est aussi possible d'utiliser :

```powershell
winget install MSYS2.MSYS2
```

Une fois MSYS2 installé, ouvrir le raccourci **MSYS2 MINGW64** dans le menu
Démarrer. Il est important de choisir **MINGW64**, pas seulement "MSYS2".

## 2. Installer les dépendances

Dans la fenêtre **MSYS2 MINGW64**, commencer par mettre MSYS2 à jour :

```bash
pacman -Syu
```

Si MSYS2 demande de fermer la fenêtre, fermer la fenêtre, rouvrir
**MSYS2 MINGW64**, puis relancer :

```bash
pacman -Syu
```

Installer ensuite tout ce dont SiteWatch a besoin :

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts \
  mingw-w64-x86_64-libssh2 mingw-w64-x86_64-zlib mingw-w64-x86_64-nlohmann-json
```

Quand MSYS2 demande confirmation, répondre `Y`.

## 3. Ouvrir le projet dans VS Code

Dans VS Code :

1. cliquer **File > Open Folder...** ;
2. choisir le dossier du projet, par exemple
   `C:\Projets\SiteWatch` ;
3. si VS Code propose d'installer des extensions recommandées, accepter.

Les extensions utiles sont :

- **CMake Tools** ;
- **CMake** ;
- **C/C++**.

Le projet contient déjà une configuration `.vscode/`, donc aucune tâche n'est
à créer manuellement.

## 4. Compiler

Dans VS Code, appuyer sur :

```text
Ctrl+Shift+B
```

Choisir la tâche :

```text
CMake: Build (MinGW)
```

Si tout est déjà compilé, le message suivant est normal :

```text
ninja: no work to do.
```

Cela veut simplement dire : "rien n'a changé, donc rien à recompiler".

L'exécutable est créé ici :

```text
build-mingw\SiteWatch.exe
```

## 5. Lancer l'application depuis VS Code

Dans VS Code :

1. ouvrir **Terminal > Run Task...** ;
2. choisir `SiteWatch: Run`.

La tâche compile d'abord si nécessaire, puis lance `SiteWatch.exe`.

Il est aussi possible de lancer directement :

```text
build-mingw\SiteWatch.exe
```

depuis l'Explorateur Windows.

## 6. Quand faut-il recompiler ?

Après une modification de code, relancer simplement :

```text
Ctrl+Shift+B
```

CMake et Ninja savent détecter les fichiers modifiés :

- un fichier `.cpp` modifié est recompilé ;
- un fichier `.h` modifié recompile les fichiers qui l'utilisent ;
- le fichier `VERSION` est surveillé par CMake ;
- les ressources Qt dans `resources/app.qrc` sont prises en compte.

Si seul `VERSION` change, la recompilation mettra à jour la version
affichée dans l'application.

## 7. Forcer une recompilation propre

En général, ce n'est pas nécessaire. Pour repartir proprement :

1. dans VS Code, ouvrir **Terminal > Run Task...** ;
2. lancer `CMake: Clean (MinGW)` ;
3. lancer ensuite `CMake: Build (MinGW)`.

Si le dossier de build est vraiment incohérent, le script
`scripts/windows/vscode-mingw.ps1` sait aussi réinitialiser `build-mingw` quand le
cache CMake pointe vers un ancien dossier du projet.

## 8. Erreurs fréquentes

### VS Code utilise le mauvais CMake

Dans la sortie CMake, vérifier la présence de :

```text
C:\msys64\mingw64\bin\cmake.exe
```

Si la sortie affiche plutôt :

```text
C:\Program Files\CMake\bin\cmake.exe
```

recharger la fenêtre VS Code avec **Developer: Reload Window**, puis relancer la
configuration CMake.

### `rcc` échoue avec `0xc0000135`

Cela veut dire que Qt est lancé sans les DLL MSYS2 dans le `PATH`.

Le projet configure déjà le bon environnement dans `.vscode/settings.json` et
`CMakePresets.json`. Recharger VS Code, puis relancer :

```text
CMake: Configure
```

### MSYS2 introuvable

Le script cherche MSYS2 ici :

```text
C:\msys64
```

Si MSYS2 est installé ailleurs, définir la variable d'environnement
`MSYS2_ROOT` vers le dossier racine de MSYS2.

Exemple :

```powershell
$env:MSYS2_ROOT = "D:\msys64"
```

## 9. Compiler sans VS Code

Pour compiler au terminal, ouvrir **MSYS2 MINGW64**, aller dans le dossier du
projet, puis lancer :

```bash
cmake --preset mingw
cmake --build --preset mingw
./build-mingw/SiteWatch.exe
```

Dans MSYS2, les chemins Windows s'écrivent différemment. Par exemple :

```text
C:\Projets\SiteWatch
```

devient :

```text
/c/Projets/SiteWatch
```

## 10. Et maintenant ?

SiteWatch démarre correctement ?
Passer maintenant au [Guide utilisateur](GUIDE.md) pour configurer un
premier site, synchroniser les journaux et commencer l'analyse.
