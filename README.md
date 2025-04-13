
# Mini Système de Fichiers en C

## Présentation

Ce projet consiste en la réalisation d’un simulateur de système de fichiers en langage C, développé dans le cadre du cursus **Licence 3 Informatique**. Il permet de simuler un système complet de gestion de fichiers et répertoires, incluant la création, suppression, déplacement, copie d’éléments, gestion des permissions, des liens symboliques et durs, ainsi que la lecture et écriture dans les fichiers simulés.

Toutes les opérations sont effectuées sur un fichier binaire (`filesystem.img`), sans impacter le système réel de la machine. Le système inclut un fichier de log (`log.txt`) qui enregistre toutes les opérations effectuées pour faciliter le suivi et la gestion des erreurs.

## Équipe Projet

- **Meriem BOUAZZAOUI** (31%)
- **Eliarisoa ANDRIANTSITOHAINA** (33%)
- **Mario RAZAFINONY** (36%)

## Contenu de l'archive

L’archive compressée (au format tar compressé `.tgz`) contient exclusivement les éléments suivants :

- `filesystem.c` — Code source principal du système de fichiers, détaillé et commenté.
- `Makefile` — Automatisation de la compilation et installation.
- `README.md` — Guide d’installation et utilisation du logiciel (ce document).
- Un document PDF décrivant les choix des structures de données et algorithmes utilisés.

---

## Guide d’installation

### Prérequis

- Système UNIX/Linux
- Compilateur C (recommandé : `gcc`)

### Compilation et installation

Depuis le répertoire contenant les fichiers sources, lancez :

```bash
make
```

Pour installer le logiciel (assurez-vous d’avoir les droits nécessaires) :

```bash
make install
```

---

## Lancement et utilisation

### Initialisation d’un nouveau système

```bash
./filesystem --i
```

### Chargement du système existant

```bash
./filesystem
```

---

## Commandes disponibles

| Commande | Description |
|---|---|
| `help` | Affiche l'aide |
| `ls` | Liste le contenu du répertoire courant |
| `pwd` | Affiche le répertoire courant |
| `cd <path>` | Change de répertoire |
| `mkdir <dir>` | Crée un nouveau répertoire |
| `touch <file>` | Crée un fichier vide |
| `rm <file>` | Supprime un fichier |
| `remdir <dir>` | Supprime un répertoire (récursif) |
| `cp <src> <newname> <dest_path>` | Copie un fichier ou répertoire |
| `mv <src> <dest_path>` | Déplace un fichier ou répertoire |
| `ln <filename> <linkname> <target_path>` | Crée un lien dur |
| `sym <target_path> <linkname>` | Crée un lien symbolique |
| `stat <file>` | Affiche les infos détaillées d’un fichier |
| `chmod <file> <permissions>` | Modifie les permissions |
| `wfile <filename> <mode> <texte>` | Écrit dans un fichier (`add` ou `rewrite`) |
| `rfile <filename>` | Lit et affiche le contenu du fichier |
| `exit` | Quitte et sauvegarde l'état du système |

---

## Structure des données principales

- `Inode` : Gestion des métadonnées (type, taille, permissions, timestamps, blocs associés).
- `DirectoryEntry` et `Directory` : Gestion du contenu des répertoires.
- `OpenFile` : Gestion des fichiers ouverts (positions de lecture/écriture).

---

## Logging et gestion d'erreurs

Toutes les opérations sont journalisées dans un fichier texte (`log.txt`). Ce fichier contient des entrées détaillées pour chaque opération, facilitant ainsi le débogage et la traçabilité.

Exemple d'entrée de log :

```
[Fichier créé] Nom : exemple.txt, Permissions : rwx, Répertoire parent : inode 2
```

Les erreurs critiques sont également affichées sur la sortie standard pour alerter immédiatement l'utilisateur.

---

## Exemple de structure initiale

Après initialisation, voici la structure de base du système simulé :

```
/
├── home/ (répertoire courant par défaut)
└── usr/
    └── local/
```

---

## Remarques importantes

- **Simulation complète** : Aucun impact sur votre système de fichiers réel.
- **Sauvegarde automatique** : L'état est sauvegardé après chaque opération.
- **Gestion de la concurrence** : Utilisation du verrouillage pour gérer les accès simultanés de manière sécurisée.

---

© Projet académique – Licence 3 Informatique – 2024/2025
