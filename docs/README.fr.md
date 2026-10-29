# Windows Lite Restore

Outil natif pour Windows 10/11 (x64) qui restaure les services, les stratégies
de registre, les tâches planifiées et les fonctionnalités optionnelles que les
images Windows « Lite » désactivent ou suppriment couramment — toujours avec
révision de l'utilisateur et en utilisant les valeurs de référence Microsoft.

En plus de la restauration, l'application propose des **profils d'optimisation**
(Jeux, IA, Bureau) qui désactivent — ou passent en manuel — uniquement les
services inutiles à chaque situation. Chaque modification, qu'il s'agisse
d'activer **ou** de désactiver, passe par une révision, une simulation, un
rapport et une sauvegarde réversible.

---

## Fonctionnalités

- Interface native sans dépendance externe (Win32 uniquement).
- Onglets : **Fonctionnalités**, **Services**, **Stratégies**, **Tâches**,
  **Diagnostic** et **Journal**.
- Sélecteur de profil sous forme de boutons visibles (**Défaut Microsoft**,
  **Jeux**, **IA**, **Bureau**).
- Sélection individuelle par case à cocher, avec recherche, filtre par catégorie
  et actions rapides (« Recommandés », « Tous les éléments en attente », « Effacer »).
- **Simulation (dry-run)** pour voir exactement ce qui changera avant d'appliquer.
- **Point de restauration** et **sauvegarde réversible** (services, stratégies,
  tâches) avant d'appliquer.
- **Annuler la dernière exécution** (menu Diagnostic).
- Rapports texte et journal (log) détaillé.
- Élévation automatique via UAC (`runas`) avec la sélection conservée.

### Profils d'optimisation

| Profil | Effet |
|--------|-------|
| **Défaut Microsoft** | Restaure services, stratégies, tâches et fonctionnalités aux valeurs d'une installation complète. |
| **Jeux** | Désactive télémétrie/rapports et la livraison en arrière-plan ; passe SysMain, Windows Search, impression, notifications, etc. en manuel. |
| **Intelligence artificielle** | Identique à Jeux, et désactive en plus les services Xbox ; libère CPU/RAM/disque pour les charges d'IA. |
| **Bureau** | Désactive uniquement Xbox et la télémétrie ; conserve impression, recherche et mises à jour. |

Les profils d'optimisation **ne modifient jamais les services essentiels du
système** (démarrage, ouverture de session, sécurité et réseau de base). Ces
services apparaissent comme **« Essentiel (protégé) »** et ne peuvent pas être
désactivés, de sorte qu'une optimisation ne pourra jamais empêcher Windows de
démarrer ou d'ouvrir une session. Les services non listés dans un profil
apparaissent comme « Aucun changement ».

### Ce qui est restauré

| Catégorie | Exemples |
|-----------|----------|
| Services | Windows Update, BITS, Store/AppX, audio, thèmes, sécurité, réseau |
| Stratégies de registre | UAC, télémétrie désactivée par stratégie, blocages de services |
| Tâches planifiées | Maintenance, Windows Update, diagnostic |
| Fonctionnalités optionnelles | .NET Framework 3.5, fonctionnalités média/langue, outils |

Le catalogue complet se trouve dans `src/catalog.cpp` (avec description,
catégorie, type de démarrage de référence et indication recommandé/critique/
optionnel).

---

## Prérequis

- Windows 10 ou Windows 11, **64 bits**.
- Aucun runtime, framework ou bibliothèque supplémentaire : l'exécutable est lié
  statiquement (MinGW-w64) et fonctionne sans installation.

> Pour **installer des fonctionnalités optionnelles supprimées**, un support/ISO
> d'installation Windows de la même édition/architecture peut être nécessaire
> (dossier source configurable dans *Paramètres*).

---

## Utilisation

1. Lancez `WindowsLiteRestore.exe`.
2. Si Windows demande l'élévation (UAC), acceptez-la — de nombreux changements
   nécessitent des privilèges d'administrateur. Sans élévation, l'application
   fonctionne en **mode lecture seule**.
3. L'analyse s'exécute automatiquement. Choisissez le **profil** souhaité via les
   boutons sous les onglets et révisez les éléments cochés.
4. Cochez éventuellement **Simuler** pour valider le plan sans rien changer.
5. Cliquez sur **Appliquer la sélection** (éléments cochés) ou **Appliquer le
   profil** / **Restaurer les valeurs par défaut Microsoft** (recommandations).
6. Confirmez le plan et choisissez de créer un point de restauration et une
   sauvegarde.
7. Pour annuler services/stratégies/tâches, utilisez *Diagnostic → Annuler la
   dernière exécution*.

### Options de ligne de commande

| Option | Description |
|--------|-------------|
| `--scan-report <fichier>` | Génère un rapport d'analyse et quitte (sans interface). |
| `--profile <nom>` | Profil initial : `default`/`padrao`, `gaming`/`jogos`, `ai`/`ia`, `office`/`escritorio`. |
| `--no-elevate` / `--read-only` | Démarre sans demander l'élévation (lecture seule). |
| `--elevated` | Indique que le processus est déjà élevé (usage interne). |
| `--resume-selection <fichier>` | Restaure une sélection enregistrée (usage interne). |

Exemple :

```powershell
WindowsLiteRestore.exe --scan-report rapport.txt --profile gaming
```

---

## Compilation

La seule exigence de compilation est un compilateur **MinGW-w64** (recommandé
[w64devkit](https://github.com/skeeto/w64devkit), qui inclut `g++` et `windres`).

```powershell
# Option A : définir le chemin du toolchain et compiler
$env:W64DEVKIT = "C:\chemin\vers\w64devkit"
.\build.ps1

# Option B : télécharger le toolchain automatiquement (nécessite internet)
.\build.ps1 -DownloadToolchain
```

Le script `build.ps1` :

1. Localise `g++.exe`/`windres.exe` (via `$env:W64DEVKIT`, `tools\w64devkit` ou `PATH`).
2. Compile les ressources (`resources.rc`, manifeste, icône, version).
3. Compile `main.cpp`, `ui.cpp`, `core.cpp` et `catalog.cpp`.
4. Lie statiquement et écrit `dist\WindowsLiteRestore.exe`, le `README.md` et le
   `SHA256.txt`.

L'icône peut être régénérée avec `tools\make-icon.ps1` (utilise System.Drawing,
sans dépendance externe).

### Structure du projet

```
src/
  app.h            types, modèle, classe App, contrats entre modules
  catalog.cpp      catalogue des services, fonctionnalités, stratégies et tâches
  core.cpp         analyse, application, sauvegarde, annulation, rapport, élévation, DISM
  ui.cpp           interface graphique (Win32 natif)
  main.cpp         point d'entrée (wWinMain)
  resources.rc     dialogues, icône, manifeste, version
  resource.h       identifiants de ressources/commandes
  app.manifest     asInvoker, DPI aware, contrôles communs v6
dist/              exécutable final + README + SHA256
tools/             make-icon.ps1
```

---

## Sécurité et limitations

- **Les valeurs de référence** correspondent à une installation complète de
  Windows 10/11. Les éditions, builds et le matériel varient ; révisez avant
  d'appliquer.
- **Les profils d'optimisation désactivent des services** (télémétrie, Xbox,
  etc.) et en passent d'autres en manuel. Ils **ne** modifient **pas** les
  services critiques ou de sécurité, et tout peut être annulé via la
  sauvegarde/Annuler.
- **Les services essentiels sont protégés** et ne peuvent jamais être désactivés.
- **Les services absents ne sont pas « installés »** par l'application — l'image
  Lite a pu supprimer le composant source. Ces éléments apparaissent comme
  indisponibles.
- **Les fonctionnalités supprimées** peuvent nécessiter un support/ISO compatible
  (erreurs DISM courantes : `0x800F081F`, `0x800F0954`, `0x800F0906` — indiquez
  le dossier source).
- **Les fonctionnalités optionnelles ne sont pas annulées** par « Annuler », afin
  d'éviter de retirer des composants qui pourraient déstabiliser le système.
- **Certains services sont protégés** (ex. Windows Update Medic, Defender). Si un
  changement échoue, vérifiez la protection contre les falsifications.
- L'application est indépendante et **sans affiliation avec Microsoft**. Utilisez-la
  à vos risques, de préférence avec un point de restauration et une sauvegarde.

### Emplacement des données

| Donnée | Emplacement |
|--------|-------------|
| Paramètres | `%LOCALAPPDATA%\WindowsLiteRestore\settings.ini` |
| Journaux (lecture seule) | `%LOCALAPPDATA%\WindowsLiteRestore\logs` |
| Journaux (élevé) | `%PROGRAMDATA%\WindowsLiteRestore\logs` |
| Sauvegardes | `%PROGRAMDATA%\WindowsLiteRestore\Backups` |
| Rapports | `%PROGRAMDATA%\WindowsLiteRestore\reports` |

---

## Avertissement

Ce logiciel modifie la configuration du système et installe des composants
Windows. Créez toujours un point de restauration et une sauvegarde réversible
avant d'appliquer, et lancez une simulation pour réviser le plan.
