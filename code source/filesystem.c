/**
 *
 * @brief Structures et définitions pour simuler un système de fichiers
 *        avec gestion des inodes, répertoires, et fichiers ouverts.
 *
 *      Pourcentage de participation:
 *          Meriem BOUAZZAOUI :             31%
 *          Eliarisoa ANDRIANTSITOHAINA :   33%
 *          Mario RAZAFINONY :              36%
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <sys/file.h>
 #include <unistd.h>
 
 #define MAX_FILE_NAME 255              /**< Taille maximum d'un nom de fichier */
 #define NUM_BLOCKS 1024                /**< Nombre de blocs dans la partition simulée */
 #define BLOCK_SIZE 512                 /**< Taille d'un bloc en octets */
 #define NUM_INODES 256                 /**< Nombre maximal d'inodes disponibles */
 #define NUM_DIRECTORY_ENTRIES 256      /**< Nombre maximal d'entrées dans un répertoire */
 #define MAX_FILE_OPEN 64               /**< Nombre maximal de fichiers ouverts simultanément */
 
 /**
  * @brief Représente un inode dans le système de fichiers simulé.
  */
 typedef struct inode {
     int id;                            /**< Identifiant unique de l'inode */
     int type;                          /**< Type d'inode : 0 = répertoire, 1 = fichier, 2 = lien symbolique */
     int size;                          /**< Taille du fichier (en octets) */
     time_t creation_time;              /**< Date et heure de création du fichier */
     time_t modification_time;          /**< Date et heure de dernière modification */
     char permissions[3];               /**< Permissions (r: lecture, w: écriture, x: exécution) */
     int blocks[NUM_BLOCKS];            /**< Tableau des indices des blocs de données associés */
     int link_count;                    /**< Nombre de liens durs vers cet inode */
     int inode_rep_parent;              /**< Indice de l'inode du répertoire parent */
 } Inode;
 
 /**
  * @brief Représente une entrée dans un répertoire.
  */
 typedef struct directory_entry {
     char filename[MAX_FILE_NAME];  /**< Nom du fichier ou du sous-répertoire */
     int inode_index;               /**< Indice de l'inode correspondant à cette entrée */
 } DirectoryEntry;
 
 /**
  * @brief Représente un répertoire.
  */
 typedef struct directory {
     DirectoryEntry entries[NUM_DIRECTORY_ENTRIES]; /**< Tableau d'entrées du répertoire */
 } Directory;
 
 /**
  * @brief Représente un fichier actuellement ouvert dans le système.
  */
 typedef struct {
     int inode;          /**< Numéro d'inode du fichier ouvert */
     int tete_lecture;   /**< Position actuelle de la tête de lecture dans le fichier */
 } OpenFile;
 
 /**
  * @brief Représente l'ensemble du système de fichiers simulé.
  */
 typedef struct filesystem {
     FILE *file;                         /**< Fichier simulant la partition sur disque */
     FILE *log;                          /**< Fichier de logs pour les opérations du système */
     Inode inodes[NUM_INODES];           /**< Tableau contenant tous les inodes du système */
     Directory root_dir;                 /**< Répertoire racine */
     Directory directories[NUM_INODES];  /**< Tableau des répertoires indexés par les indices d'inodes */
     int free_blocks[NUM_BLOCKS];        /**< Tableau indiquant l'état libre ou occupé des blocs de données */
     int current_dir;                    /**< Indice de l'inode du répertoire courant */
     OpenFile opened_file[MAX_FILE_OPEN];/**< Tableau des fichiers actuellement ouverts indexés par descripteur */
 } Filesystem;

Filesystem fs;  // Instance globale du système de fichiers

/**
 * @brief Initialise le système de fichiers à partir d'un fichier simulé.
 *
 * @param filename Le nom du fichier représentant la partition simulée.
 */
void init_filesystem(const char *filename) {
    fs.file = fopen(filename, "wb+");  // Ouverture en mode binaire
    fs.log = fopen("log.txt", "a");   // Création du fichier texte pour les log
    if (!fs.file) {
        perror("Erreur lors de l'ouverture du fichier système de fichiers");
        exit(1);
    }

    // Initialisation des blocs libres
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fs.free_blocks[i] = 0;  // 0 = libre
    }

    // Initialisation des inodes (simuler les inodes vides)
    for (int i = 0 ; i < NUM_INODES ; i++) {
        fs.inodes[i].id = i;
        fs.inodes[i].size = -1;
        fs.inodes[i].type = -1;
        fs.inodes[i].creation_time = time(NULL);
        fs.inodes[i].modification_time = time(NULL);
        fs.inodes[i].inode_rep_parent = -1;
        memset(fs.inodes[i].permissions, 0, 3);
        memset(fs.inodes[i].blocks, -1, NUM_BLOCKS * sizeof(int));  // Bloc non alloué
        fs.inodes[i].link_count = 0;  // Aucun lien
        for (int j = 1 ; j < NUM_DIRECTORY_ENTRIES ; j++){
            fs.directories[i].entries[j].inode_index = -1;
            memset(fs.directories[i].entries[j].filename, 0, MAX_FILE_NAME * sizeof(char));
        }
    }

    // Marquer qu'aucun fichier n'est ouvert
    for (int i ; i < MAX_FILE_OPEN ; i++) {
        fs.opened_file[i].inode = -1;
        fs.opened_file[i].tete_lecture = -1;
    }

    // Remplissage du fichier binaire pour y ecrire plus tard
    char buf[2];
    buf[0] = '\0';
    for(int i = sizeof(Filesystem) ; i < sizeof(Filesystem) + NUM_BLOCKS*BLOCK_SIZE ; i++){
        fseek(fs.file, i, SEEK_SET);
        fwrite(buf, sizeof(char), 1, fs.file);
    }


    // Initialisation du répertoire racine
    Directory root;
    for(int i = 0 ; i < NUM_DIRECTORY_ENTRIES ; i++){
        memset(root.entries[i].filename, 0, MAX_FILE_NAME * sizeof(char));
        root.entries[i].inode_index = -1;
    }
    fs.root_dir = root;

    fs.directories[0] = fs.root_dir;
    fs.inodes[0].size = 0;
    fs.inodes[0].type = 0;
    fs.inodes[0].creation_time = time(NULL);
    fs.inodes[0].modification_time = time(NULL);
    fs.inodes[0].inode_rep_parent = 0;
    strncpy(fs.inodes[0].permissions, "rwx", 3);
    fs.current_dir = 0;

    // printf("fs size : %d\n", sizeof(Filesystem));

    fclose(fs.file);

    // Ouverture du fichier en mode ecriture/lecture
    fs.file = fopen(filename, "rb+");

    fprintf(fs.log,"\nNouveau système initialisé\n");

    printf("Système initialisé avec succès\n");
}

/**
 * @brief Alloue un bloc libre dans le système de fichiers.
 *
 * @return L'index du bloc alloué ou -1 si aucun bloc n'est disponible.
 */
int allocate_block() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (fs.free_blocks[i] == 0) {
            fs.free_blocks[i] = 1;  // Marquer le bloc comme alloué
            fprintf(fs.log,"\nAllocation du bloc %d \n",i);
            return i;
        }
    }
    fprintf(fs.log,"\nEchec d'allocation\n");
    return -1;  // Aucun bloc libre trouvé
}

/**
 * @brief Libère un bloc précédemment alloué.
 *
 * @param block_index L'index du bloc à libérer.
 */
void free_block(int block_index) {
    if (block_index >= 0 && block_index < NUM_BLOCKS) {
        fs.free_blocks[block_index] = 0;  // Marquer le bloc comme libre
        fprintf(fs.log,"\nLibération du bloc %d\n",block_index);
    } else {
        printf("Erreur: tentative de libération d'un bloc invalide (%d).\n", block_index);
        fprintf(fs.log,"\nEchec de la libération du bloc %d\n", block_index);
    }
}



/**
 * @brief Recherche l'inode correspondant à un nom de fichier dans un répertoire donné.
 *
 * @param filename Le nom du fichier recherché.
 * @param dir Le répertoire dans lequel effectuer la recherche.
 * @return L'index de l'inode correspondant, ou -1 si introuvable.
 */
int rechInode(const char *filename, Directory dir){
    int inode = -1;
    int i = 0;

    //chercher le numero d'inode du fichier dans le repertoire
    while (i<NUM_DIRECTORY_ENTRIES && inode==-1){
        if (strcmp(filename, dir.entries[i].filename) == 0){
            inode = dir.entries[i].inode_index;
        }
        i++;
    }
    return inode;
    fprintf(fs.log,"\nNuméro d'inode de %s : %d\n", filename, inode);
}

/**
 * @brief Recherche une entrée libre dans un répertoire donné.
 *
 * @param dir_inode L'index de l'inode du répertoire à examiner.
 * @return L'index de l'entrée libre trouvée, ou -1 si aucune n'est disponible.
 */
int rechEntree(int dir_inode){
    int index = -1;
    int i = 0;
    Directory dir = fs.directories[dir_inode];

    // Chercher une entrée libre
    while(i<NUM_DIRECTORY_ENTRIES && index==-1){
        if (dir.entries[i].inode_index == -1){
            index = i;
        }
        i++;
    }
    return index;
    fprintf(fs.log,"\nEntrée de répertoire trouvé : %d\n", index);
}

/**
 * Vérifie si un inode donné possède une permission spécifique.
 *
 * @param inode_index L'index de l'inode à vérifier.
 * @param perm Le caractère de permission ('r' pour read, 'w' pour write, 'x' pour execute).
 * @return 1 si l'inode possède la permission, 0 sinon.
 */
 int has_permission(int inode_index, char perm) {
    // Vérification que l'inode est valide
    if (inode_index < 0 || inode_index >= NUM_INODES) {
        return 0;
    }

    Inode *inode = &fs.inodes[inode_index];
    fprintf(fs.log,"\nPermissions du fichier d'inode %d : %s\n", inode_index, inode->permissions);

    // Vérification de la présence du caractère de permission dans la chaîne de permissions
    if (perm == 'r' && strchr(inode->permissions, 'r')) return 1;
    if (perm == 'w' && strchr(inode->permissions, 'w')) return 1;
    if (perm == 'x' && strchr(inode->permissions, 'x')) return 1;

    return 0;  // Permission refusée
}

/**
 * @brief Modifie les permissions (r, w, x) d'un fichier ou répertoire.
 * @param filename Le nom du fichier/répertoire.
 * @param newPerms Chaîne de 3 caractères (ex.: "rw-", "r-x", etc.).
 * @param dir_inode L'inode du répertoire courant ou parent.
 * @return 0 si succès, -1 si erreur.
 */
 int change_permissions(const char *filename, const char *newPerms, int dir_inode) {
    // 1) Retrouver l'inode du fichier/répertoire
    int inode_index = rechInode(filename, fs.directories[dir_inode]);
    if (inode_index == -1) {
        fprintf(fs.log, "\nErreur lors du changement de permissions sur le fichier %s\n", filename);
        printf("Erreur : '%s' introuvable dans ce répertoire.\n", filename);
        return -1;
    }

    // 2) Mettre à jour les permissions (3 caractères max)
    Inode *node = &fs.inodes[inode_index];
    strncpy(node->permissions, newPerms, 3);
    node->modification_time = time(NULL);

    fprintf(fs.log, "\nNouvelles permissions pour le fichier %s : %s\n", filename, newPerms);

    printf("Permissions de '%s' modifiées en '%s'.\n", filename, newPerms);
    return 0;
}



/**
 * @brief Crée un nouveau fichier dans un répertoire spécifié.
 *
 * Cette fonction vérifie les permissions d'écriture du répertoire parent avant de créer un nouveau fichier.
 * Elle vérifie également qu'aucun fichier portant le même nom n'existe déjà dans le répertoire. Ensuite,
 * elle alloue un inode et un bloc de données pour le nouveau fichier, initialise ses métadonnées et ajoute
 * une entrée dans le répertoire parent.
 *
 * @param filename Nom du fichier à créer.
 * @param permissions Chaîne de caractères représentant les permissions initiales du fichier (ex : "rw-", "rwx").
 * @param dir_inode Index de l'inode du répertoire parent où le fichier doit être créé.
 *
 * @return L'index de l'inode du fichier créé en cas de succès, ou -1 en cas d'erreur (permissions insuffisantes,
 *         absence d'espace ou d'inode disponible, fichier déjà existant, etc.).
 */
int create_file(const char *filename, const char *permissions, int dir_inode) {
    // Vérifier permission 'w' sur le répertoire parent
    if (!has_permission(dir_inode, 'w')) {
        fprintf(fs.log, "\nErreur sur la création du fichier %s\n", filename);
        printf("Erreur : permission insuffisante pour créer un fichier dans ce répertoire.\n");
        return -1;
    }

    // Répertoire où on va créer le fichier
    Directory *dir = &fs.directories[dir_inode];
    int inode_index = -1;

    // Vérifier si le fichier existe dans le répertoire
    if (rechInode(filename, *dir) != -1){
        fprintf(fs.log, "\nErreur sur la création du fichier %s\n", filename);
        printf("Erreur de création, un fichier de même nom existe déjà dans le répertoire\n");
        return -1;
    }

    // Trouver un inode libre
    for (int i = 0; i < NUM_INODES; i++) {
        if (fs.inodes[i].size == -1) { 
            inode_index = i;
            fs.inodes[i].size = 0; // Marquer comme utilisé
            break;
        }
    }

    // Vérifier si on a trouvé un inode libre
    if (inode_index == -1) {
        fprintf(fs.log, "\nErreur sur la création du fichier %s\n", filename);
        printf("Erreur: Aucun inode libre.\n");
        return -1;
    }

    Inode *inode = &fs.inodes[inode_index];

    // Allouer uniquement 1 bloc pour commencer
    int block = allocate_block();
    if (block == -1) {
        fprintf(fs.log, "\nErreur sur la création du fichier %s\n", filename);
        printf("Erreur: Pas de blocs libres disponibles.\n");
        inode->size = -1; // Marquer l'inode comme inutilisé
        return -1;
    }

    // Chercher un espace libre dans le répertoire
    int index_rep = rechEntree(dir_inode);
    if(index_rep == -1){
        fprintf(fs.log, "\nErreur sur la création du fichier %s\n", filename);
        printf("Erreur: Aucun espace dans le répertoire.\n");
        free_block(block); // Libérer le bloc alloué
        inode->size = -1; // Marquer l'inode comme inutilisé
        return -1;
    }

    // Ajouter le fichier au répertoire
    strncpy(dir->entries[index_rep].filename, filename, MAX_FILE_NAME);
    dir->entries[index_rep].inode_index = inode_index;
    fprintf(fs.log, "\nFichier %s créé avec les permissions %s dans le repertoire d'inode %d\n", filename, permissions, dir_inode);
    printf("Fichier '%s' créé avec succès.\n", filename);

    // Initialiser l'inode
    inode->blocks[0] = block;
    inode->type = 1;
    inode->creation_time = time(NULL);
    inode->modification_time = time(NULL);
    inode->inode_rep_parent = dir_inode;
    strncpy(inode->permissions, permissions, 3);

    //printf("block : %d\n", block);

    return inode_index;
}

/**
 * @brief Supprime un fichier spécifié d'un répertoire.
 *
 * Cette fonction vérifie l'existence du fichier indiqué dans le répertoire spécifié par son inode. 
 * Si le fichier existe, elle libère tous les blocs associés au fichier, réinitialise l'inode correspondant 
 * pour le rendre disponible et supprime l'entrée correspondante dans le répertoire parent. Elle prend également
 * en compte les éventuels liens durs vers ce fichier.
 *
 * @param filename Nom du fichier à supprimer.
 * @param dir_inode Index de l'inode du répertoire contenant le fichier à supprimer.
 *
 * @note Cette fonction ne peut pas supprimer un répertoire.
 */
void delete_file(char *filename, int dir_inode) {
    // Répertoire où le fichier se situe
    Directory *dir = &fs.directories[dir_inode];
    int inode_index = rechInode(filename, *dir);

    // Vérifier si le fichier existe
    if(inode_index == -1){
        fprintf(fs.log, "\nErreur sur la suppression du fichier %s\n", filename);
        printf("Erreur : Fichier inexistant.\n");
    } else if(fs.inodes[inode_index].type != 1 && fs.inodes[inode_index].type != 2){
        fprintf(fs.log, "\nErreur sur la suppression du fichier %s\n", filename);
        printf("Erreur : Type de fichier non reconnu ou est un répertoire.\n");
    } else {
        Inode *inode = &fs.inodes[inode_index];

        // Libérer tous les blocs associés
        for (int i = 0; i < NUM_BLOCKS; i++) {
            if (inode->blocks[i] != -1) {
                free_block(inode->blocks[i]);
                inode->blocks[i] = -1;
            }
        }

        inode->size = -1; // Marquer l'inode comme libre
        inode->type = -1;
        inode->creation_time = time(NULL);
        inode->modification_time = time(NULL);
        inode->link_count = 0;
        inode->inode_rep_parent = -1;

        // Supprimer l'entrée du répertoire
        int i = 0;
        while (inode_index != -1) {
            if (dir->entries[i].inode_index == inode_index && strcmp(filename,dir->entries[i].filename) == 0) { // Vérifier le nom du fichier au cas où on a un lien dur
                dir->entries[i].inode_index = -1;
                memset(dir->entries[i].filename, 0, MAX_FILE_NAME);
                inode_index = -1;
            }
            i++;
        }

        printf("Fichier %s supprimé avec succès.\n", filename);
        fprintf(fs.log, "\nFichier %s supprimé\n", filename);
    }
}



/**
 * @brief Supprime un répertoire vide du système de fichiers.
 *
 * @param dirname    Le nom du répertoire à supprimer.
 * @param parent_dir L'inode du répertoire parent (celui qui contient dirname).
 * @return 0 si la suppression réussit, -1 en cas d'erreur.
 */
int delete_directory(const char *dirname, int parent_dir) {
    // 1) Trouver l'inode du répertoire à supprimer en cherchant dirname dans le répertoire parent
    int dir_inode = rechInode(dirname, fs.directories[parent_dir]);
    if (dir_inode == -1) {
        fprintf(fs.log, "\nErreur sur la suppression du répertoire %s\n", dirname);
        printf("Erreur: Le répertoire '%s' n'existe pas dans le répertoire %d.\n", dirname, parent_dir);
        return -1;
    }

    // 2) Vérifier que c'est bien un répertoire
    if (fs.inodes[dir_inode].type != 0) {  // 0 = répertoire
        fprintf(fs.log, "\nErreur sur la suppression du répertoire %s\n", dirname);
        printf("Erreur: '%s' n'est pas un répertoire.\n", dirname);
        return -1;
    }
    // 3) Vérifier permission 'w' sur ce répertoire
    if (!has_permission(dir_inode, 'w')) {
        fprintf(fs.log, "\nErreur sur la suppression du répertoire %s\n", dirname);
        printf("Erreur : pas de permission d'écriture sur le répertoire '%s'.\n", dirname);
        return -1;
    }

    // 4) Supprimer récursivement
    Directory *dir_to_delete = &fs.directories[dir_inode];
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        if (dir_to_delete->entries[i].inode_index != -1) {
            int inode = dir_to_delete->entries[i].inode_index;
            //printf("inode %d\n", inode);
            if (fs.inodes[inode].type == 0){
                delete_directory(dir_to_delete->entries[i].filename, dir_inode);
            }

            if (fs.inodes[inode].type == 1 || fs.inodes[inode].type == 2){
                delete_file(dir_to_delete->entries[i].filename, dir_inode);
            }
        }
    }

    // 4) Supprimer l'entrée correspondant à ce répertoire dans le parent
    Directory *parent_directory = &fs.directories[parent_dir];
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        if (parent_directory->entries[i].inode_index == dir_inode &&
            strcmp(parent_directory->entries[i].filename, dirname) == 0)
        {
            parent_directory->entries[i].inode_index = -1;
            memset(parent_directory->entries[i].filename, 0, MAX_FILE_NAME);
            break;
        }
    }
    
           
    // 5) Libérer l'inode du répertoire
    Inode *inode_ptr = &fs.inodes[dir_inode];
    inode_ptr->size = -1;
    inode_ptr->type = -1;
    inode_ptr->creation_time = time(NULL);
    inode_ptr->modification_time = time(NULL);
    inode_ptr->inode_rep_parent = -1;
    inode_ptr->link_count = 0;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (inode_ptr->blocks[i] != -1) {
            // Normalement, un répertoire n'a pas de blocs de données, mais par sécurité :
            free_block(inode_ptr->blocks[i]);
            inode_ptr->blocks[i] = -1;
        }
    }

    printf("Le répertoire '%s' a été supprimé avec succès.\n", dirname);
    fprintf(fs.log, "\nSuccès de la suppression du répertoire %s\n", dirname);
    return 0;
}


/**
 * Crée un répertoire dans le système de fichiers simulé.
 *
 * @param dirname Nom du répertoire à créer.
 * @return L'index de l'inode du répertoire créé, ou -1 en cas d'erreur.
 */
 int create_directory(const char *dirname, int inode_dir) {
    int inode_index = -1;

    // Rechercher un inode libre pour stocker le répertoire
    int i = 0;
    while (i < NUM_INODES && inode_index == -1) {
        if (fs.inodes[i].size == -1) {  // Un inode libre a une taille de -1
            inode_index = i;
        }
        i++;
    }

    // Vérifier si un inode a été trouvé
    if (inode_index == -1) {
        fprintf(fs.log, "\nErreur sur la création du répertoire %s\n", dirname);
        printf("Erreur: Aucun inode libre pour créer un répertoire.\n");
        return -1;
    }

    Directory *dir = &fs.directories[inode_dir];

    //Vérifier si le fichier existe dans le répertoire
    if (rechInode(dirname, *dir) != -1){
        fprintf(fs.log, "\nErreur sur la création du répertoire %s\n", dirname);
        printf("Erreur de création, un fichier de même nom existe déjà dans le répertoire\n");
        return -1;
    }

    // Chercher une entrée libre dans le répertoire
    int index = rechEntree(inode_dir);
    if (index == -1){
        fprintf(fs.log, "\nErreur sur la création du répertoire %s\n", dirname);
        printf("Erreur: Pas d'espace libre dans le répertoire.\n");
        return -1;
    }

    Directory *new_dir = &fs.directories[inode_index];

    // Initialisation de l'inode pour le répertoire
    Inode *inode = &fs.inodes[inode_index];
    inode->size = 0;  // Un répertoire commence vide
    inode->type = 0;
    inode->inode_rep_parent = inode_dir;
    inode->creation_time = time(NULL);
    inode->modification_time = time(NULL);
    strncpy(inode->permissions, "rwx", 3);  // Lecture, écriture et exécution
    inode->link_count = 1;  // Le répertoire est lié à lui-même

    // Aucun bloc n'est alloué directement
    for (int i = 0; i < NUM_BLOCKS; i++) {
        inode->blocks[i] = -1;
    }

    //Initialiser les entrées du répertoire
    for(int i = 0 ; i < NUM_DIRECTORY_ENTRIES ; i++){
        memset(new_dir->entries[i].filename, 0, MAX_FILE_NAME * sizeof(char));
        new_dir->entries[i].inode_index = -1;
    }

    // Ajouter le répertoire au répertoire parent
    strncpy(dir->entries[index].filename, dirname, MAX_FILE_NAME);
    dir->entries[index].inode_index = inode_index;
    fprintf(fs.log, "\nRépertoire '%s' créé avec succès\n", dirname);
    printf("Répertoire '%s' créé avec succès.\n", dirname);
    return inode_index;
}






/**
 * Déplace un répertoire (et son contenu) d'un répertoire parent source vers un répertoire parent destination.
 *
 * @param srcDirName   Le nom du répertoire à déplacer.
 * @param srcParentDir L'inode du répertoire parent source.
 * @param dstParentDir L'inode du répertoire parent destination.
 * @return 0 si la réussite, -1 si erreur.
 */
 int move_directory(const char *srcDirName, int srcParentDir, int dstParentDir) {
    // 1) Récupérer l'inode du répertoire source
    int srcDirInode = rechInode(srcDirName, fs.directories[srcParentDir]);
    if (srcDirInode == -1) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : Le répertoire '%s' n'existe pas dans le répertoire %d.\n", srcDirName, srcParentDir);
        return -1;
    }

    // 2) Vérifier que c'est bien un répertoire
    if (fs.inodes[srcDirInode].type != 0) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : '%s' n'est pas un répertoire.\n", srcDirName);
        return -1;
    }
     // 3) Vérifier la permission 'w' sur le répertoire source (pour supprimer l'entrée)
     if (!has_permission(srcParentDir, 'w')) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : permission d'écriture refusée dans le répertoire source (inode %d).\n", srcParentDir);
        return -1;
    }

    // 4) Vérifier qu'il n'y a pas déjà un répertoire (ou fichier) du même nom dans la destination
    if (rechInode(srcDirName, fs.directories[dstParentDir]) != -1) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : Le nom '%s' existe déjà dans le répertoire %d.\n", srcDirName, dstParentDir);
        return -1;
    }
    // 5) Vérifier la permission 'w' sur le répertoire cible (pour créer l'entrée)
    if (!has_permission(dstParentDir, 'w')) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : permission d'écriture refusée dans le répertoire cible (inode %d).\n", dstParentDir);
        return -1;
    }

    // 6) Ajouter une entrée dans le répertoire destination
    int dstIndex = rechEntree(dstParentDir);
    if (dstIndex == -1) {
        fprintf(fs.log, "\nErreur sur le déplacement du répertoire %s\n", srcDirName);
        printf("Erreur : Pas d'espace libre dans le répertoire %d.\n", dstParentDir);
        return -1;
    }
    Directory *destDir = &fs.directories[dstParentDir];
    strncpy(destDir->entries[dstIndex].filename, srcDirName, MAX_FILE_NAME);
    destDir->entries[dstIndex].inode_index = srcDirInode;

    // 7) Supprimer l'entrée du répertoire source
    Directory *sourceDir = &fs.directories[srcParentDir];
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        if (sourceDir->entries[i].inode_index == srcDirInode &&
            strcmp(sourceDir->entries[i].filename, srcDirName) == 0)
        {
            sourceDir->entries[i].inode_index = -1;
            memset(sourceDir->entries[i].filename, 0, MAX_FILE_NAME);
            break;
        }
    }

    // 8) Mettre à jour l'inode du répertoire pour pointer vers son nouveau parent
    fs.inodes[srcDirInode].inode_rep_parent = dstParentDir;
    fs.inodes[srcDirInode].modification_time = time(NULL);

    fprintf(fs.log, "\nRépertoire '%s' (inode %d) déplacé de %d vers %d\n", srcDirName, srcDirInode, srcParentDir, dstParentDir);
    printf("Répertoire '%s' (inode %d) déplacé de %d vers %d.\n", srcDirName, srcDirInode, srcParentDir, dstParentDir);
    return 0;
}

/**
 * Retourne l'inode correspondant à un chemin (absolu ou relatif) dans le système de fichiers.
 *
 * @param path          Le chemin (ex: "/home/user/file.txt" ou "docs/fichier1.txt")
 * @param current_dir   L'inode du répertoire courant (utilisé si le chemin n'est pas absolu)
 * @return L'inode du fichier/répertoire pointé par 'path', ou -1 en cas d'erreur.
 */
 int get_inode_from_path(const char *path, int current_dir) {
    // 1) Déterminer si le chemin est absolu ou relatif
    int inode = 0;  // Par défaut, la racine
    if (path[0] != '/') {
        // Chemin relatif => partir du répertoire courant
        inode = current_dir;
    }

    // 2) Copie du chemin local pour strtok (car strtok modifie la chaîne)
    char tempPath[1024];
    memset(tempPath, 0, sizeof(tempPath));
    strncpy(tempPath, path, sizeof(tempPath) - 1);

    // 3) Découper par les "/"
    char *token = strtok(tempPath, "/");
    while (token != NULL) {
        // Ignorer les "." (rester dans le même répertoire)
        if (strcmp(token, ".") == 0) {
            // On ne change rien
        }
        else {
            // aller au repertoire parent si on a ".."
            if (strcmp(token, "..") == 0){
                inode = fs.inodes[inode].inode_rep_parent;
            } else {

                // 3.1) Chercher le token dans le répertoire inode actuel
                int foundInode = rechInode(token, fs.directories[inode]);
                if (foundInode == -1) {
                    // Pas trouvé
                    fprintf(fs.log, "\nErreur : '%s' est introuvable dans le répertoire inode %d.\n", token, inode);
                    printf("Erreur : '%s' est introuvable dans le répertoire inode %d.\n", token, inode);
                    return -1;
                }
                // 3.2) Mettre à jour l'inode actuel
                inode = foundInode;
            }
        }

        token = strtok(NULL, "/");
    }

    // 4) inode final
    
    fprintf(fs.log, "\nInode du fichier du chemin %s : %d\n", path, inode);
    return inode;
}


/**
 * Crée un lien symbolique (symlink) vers une cible (targetPath), dans le répertoire parent parentDir.
 *
 * @param linkName   Nom du lien symbolique (ex: "lien_symb")
 * @param targetPath Chemin vers la cible (ex: "/home/user/fichier.txt" ou "fichier2.txt")
 * @param parentDir  Inode du répertoire dans lequel on crée le lien symbolique
 * @return L'inode du lien symbolique créé, ou -1 en cas d'erreur
 */
 int create_symbolic_link(const char *linkName, const char *targetPath, int parentDir) {
    // 1) Vérifier si un fichier ou répertoire du même nom existe déjà dans parentDir
    int existingInode = rechInode(linkName, fs.directories[parentDir]);
    if (existingInode != -1) {
        fprintf(fs.log, "\nErreur lors de la création du lien symbolique vers %s\n", targetPath);
        printf("Erreur : Le nom '%s' existe déjà dans le répertoire inode %d.\n", linkName, parentDir);
        return -1;
    }

    // 2) Vérifier que le chemin est valide
    int path_inode = get_inode_from_path(targetPath, parentDir);
    if(path_inode == -1){
        fprintf(fs.log, "\nErreur lors de la création du lien symbolique vers %s\n", targetPath);
        printf("Erreur : le chemin n'est pas valide\n");
        return -1;
    }

    // 3) Trouver un inode libre
    int symlinkInode = -1;
    for (int i = 0; i < NUM_INODES; i++) {
        if (fs.inodes[i].size == -1) {  // -1 signifie inode libre
            symlinkInode = i;
            break;
        }
    }
    if (symlinkInode == -1) {
        fprintf(fs.log, "\nErreur lors de la création du lien symbolique vers %s\n", targetPath);
        printf("Erreur : Pas d'inode libre pour créer le lien symbolique.\n");
        return -1;
    }

    // 4) Chercher une entrée libre dans le répertoire parent
    int dirIndex = rechEntree(parentDir);
    if (dirIndex == -1) {
        fprintf(fs.log, "\nErreur lors de la création du lien symbolique vers %s\n", targetPath);
        printf("Erreur : Pas d'espace libre dans le répertoire inode %d.\n", parentDir);
        return -1;
    }

    // 5) Allouer un bloc pour stocker la chaîne de la cible (targetPath)
    int blockIndex = allocate_block();
    if (blockIndex == -1) {
        fprintf(fs.log, "\nErreur lors de la création du lien symbolique vers %s\n", targetPath);
        printf("Erreur : Pas de blocs libres pour créer le lien symbolique.\n");
        return -1;
    }

    // 6) Initialiser l'inode du lien symbolique
    Inode *inodePtr = &fs.inodes[symlinkInode];
    inodePtr->size = sizeof(targetPath)+1;                   // La 'taille' du lien peut représenter la taille de la chaîne si on veut
    inodePtr->type = 2;                   // 2 = lien symbolique
    inodePtr->creation_time = time(NULL);
    inodePtr->modification_time = time(NULL);
    inodePtr->inode_rep_parent = parentDir;
    inodePtr->link_count = 1;             // Au moins un lien (ce lien lui-même)
    strncpy(inodePtr->permissions, "rwx", 3);  // Par exemple, autoriser la lecture/exec du lien

    // Mettre tous les blocs à -1, sauf celui qu’on vient d’allouer
    for (int i = 0; i < NUM_BLOCKS; i++) {
        inodePtr->blocks[i] = -1;
    }
    inodePtr->blocks[0] = blockIndex;

    // 7) Écrire la chaîne targetPath dans le bloc alloué
        int i = 0;
        while (*(targetPath+i) != '\0'){
            fseek(fs.file, sizeof(Filesystem) + blockIndex * BLOCK_SIZE + i, SEEK_SET);
            fwrite(targetPath+i, sizeof(char), 1, fs.file);
            i++;
        }

        fseek(fs.file, sizeof(Filesystem) + blockIndex * BLOCK_SIZE + i, SEEK_SET);
        fwrite(targetPath+i, sizeof(char), 1, fs.file);
    

    // 8) Ajouter l'entrée (linkName) dans le répertoire parent
    Directory *dirPtr = &fs.directories[parentDir];
    strncpy(dirPtr->entries[dirIndex].filename, linkName, MAX_FILE_NAME);
    dirPtr->entries[dirIndex].inode_index = symlinkInode;

    
    fprintf(fs.log, "\nLien symbolique '%s' (inode %d) créé, pointant vers '%s'\n", linkName, symlinkInode, targetPath);
    printf("Lien symbolique '%s' (inode %d) créé, pointant vers '%s'.\n", linkName, symlinkInode, targetPath);
    return symlinkInode;
}



/**
 * @brief Ouvre un fichier et crée un descripteur de fichier.
 *
 * @param filename Nom du fichier à ouvrir.
 * @param dir_inode L'inode du répertoire où se trouve le fichier.
 * @return Le descripteur du fichier ouvert ou -1 en cas d'erreur.
 */
int open_file (const char *filename, int dir_inode){
    // On cherche le repertoire parent et l'inode
    Directory dir = fs.directories[dir_inode];
    int inode = rechInode(filename, dir);
    int i = 0;
    int desc = -1;

    // Vérifier que le fichier existe
    if (inode == -1){
        fprintf(fs.log, "\nErreur sur l'ouverture du fichier %s\n", filename);
        printf("Erreur : fichier non ouvert.\n");
        return -1;
    }

    // Vérifierr si le type de fichier n'est pas un répertoire
    if(fs.inodes[inode].type != 1 && fs.inodes[inode].type != 2){
        printf("Erreur : fichier es un répertoire ou non reconnu");
        fprintf(fs.log, "\nErreur sur l'ouverture du fichier %s\n", filename);
        return -1;
    }

    // On crée un nouveau descripteur de fichier
    while (desc == -1 && i<MAX_FILE_OPEN){
        if (fs.opened_file[i].inode == -1){
            fs.opened_file[i].inode = inode;
            fs.opened_file[i].tete_lecture = sizeof(Filesystem) + fs.inodes[inode].blocks[0]*BLOCK_SIZE;
            desc = i;
        }
        i++;
    }

    // Vérifier si on a réussi a créer un descripteur
    if (desc == -1){
        printf("Erreur : fichier non ouvert.\n");
        fprintf(fs.log, "\nErreur sur l'ouverture du fichier %s\n", filename);
    }
    
    fprintf(fs.log, "\nNouveau descripteur %d pour le fichier %s\n", desc, filename);
    return desc;
}

/**
 * @brief Écrit des données dans un fichier ouvert.
 *
 * @param desc Descripteur du fichier ouvert.
 * @param texte Chaîne à écrire dans le fichier.
 * @param size Nombre d'octets à écrire.
 * @return Nombre d'octets écrits ou -1 en cas d'erreur.
 */
int write_file(int desc, const char *texte, int size){
    // Vérifier si le descripteur est valide
    if (desc > MAX_FILE_OPEN || desc < 0 || fs.opened_file[desc].inode == -1){
        fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
        printf("Erreur : descrpiteur invalide\n");
        return -1;
    } else if(size < 0) {
        printf("Erreur : taille négatif\n");
        fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
        return -1;
    }
    // Vérifier la permission 'w'
    int inode_idx = fs.opened_file[desc].inode;
    if (!has_permission(inode_idx, 'w')) {
        printf("Erreur : permission d'écriture refusée.\n");
        fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
        return -1;
    }

    // Vérifier si on ecrit bien dans un fichier
    if(fs.inodes[fs.opened_file[desc].inode].type != 1){
        printf("Erreur : tente d'ecrire dans un repertoire ou dans un lien symbolique");
        fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
        return -1;
    }

    
    // tableau pour lire si on a ecrit un nouveau charactere ou non
    char texte_tmp[1];
    memset(texte_tmp, 0, sizeof(char));
    texte_tmp[0] = '\0';

    // Tete de lecture/ecriture et inode du fichier
    int lecteur = fs.opened_file[desc].tete_lecture;
    int inode = fs.opened_file[desc].inode;

    fprintf(fs.log, "\ntête de lecture en début d'écriture : %d\n", lecteur);

    // index des blocs du fichier a ecrire
    int block_index = -1;

    // taille supplémentaire du fichier
    int maj_size = 0;

    // on cherche le numéro de bloc a ecrire
    int i = 0;
    int num_block;
    while (block_index == -1 && i<NUM_BLOCKS){
        num_block = fs.inodes[inode].blocks[i];
        // Si la tete de lecture se trouve entre le bloc et le bloc suivant, on recupere le bloc
        if (sizeof(Filesystem) + num_block*BLOCK_SIZE <= lecteur && lecteur < sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
            block_index = i;
        }
        i++;
    }


    // Vérifier si la tete de lecture est bien dans le fichier
    if(block_index == -1){
        printf("Erreur : la tête de lecture n'est pas dans le fichier.\n");
        fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
        return -1;
    } else {
        // On écrit dans le bloc tant que le bloc n'est pas complet
        int j = 0;
        while (j<size && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
            // On se positionne pour ecrire
            fseek(fs.file, lecteur, SEEK_SET);
            // On lit pour verifier si il y a des caracteres ecrit (pour mettre a jour la taille)
            fread(texte_tmp, sizeof(char),1,fs.file);
            // On se repositionne pour ecrire
            fseek(fs.file, -1, SEEK_CUR);
            // On met a jour la taille si il n'y avait rien d'ecrit
            if (texte_tmp[0]  == '\0'){
                maj_size++;
                fs.inodes[inode].size = fs.inodes[inode].size + 1;
                texte_tmp[0] = '\0';
            }
            // On ecrit
            fwrite(texte+j, sizeof(char), 1, fs.file);
            j++;
            lecteur++;
        }
        int stop = 0;
        // On réitère le processus précédent tant qu'il y a de la place a ecrire
        while (j<size && !stop){
            block_index++;
            num_block = fs.inodes[inode].blocks[block_index];
            // On alloue de la memoire si il ne reste plus aucun bloc
            if (num_block == -1){
                num_block = allocate_block();
                fs.inodes[inode].blocks[block_index] = num_block;
            }

            // Vérifier si un block a été alloué
            if(num_block == -1){
                stop = 1;
                printf("Erreur : ordinateur saturé !!!! (aucun bloc disponible)\n");
                fprintf(fs.log, "\nErreur sur l'écriture dans le fichier du descripteur %d\n", desc);
            } else {
                // On se positionne dans le bloc
                lecteur = sizeof(Filesystem) + num_block*BLOCK_SIZE;
                // Même processus pour ecrire dans le bloc + maj de la taille
                while(j<size && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){ 
                    fseek(fs.file, lecteur, SEEK_SET);
                    fread(texte_tmp, sizeof(char),1,fs.file);
                    fseek(fs.file, -1, SEEK_CUR);
                    if (texte_tmp[0] == '\0'){
                        maj_size++;
                        fs.inodes[inode].size = fs.inodes[inode].size + 1;
                        texte_tmp[0] = '\0';
                    }
                    fwrite(texte+j, sizeof(char), 1, fs.file);
                    j++;
                    lecteur++;
                }
            }
        }

    }

    
    fprintf(fs.log, "\nAugmentation de la taille du fichier de %d octets\n", maj_size);

    // Mettre a jour récursivement la taille des repertoires parents
    int id_rep_parent = inode;
    while (id_rep_parent != 0){
        id_rep_parent = fs.inodes[id_rep_parent].inode_rep_parent;
        fs.inodes[id_rep_parent].size = fs.inodes[id_rep_parent].size + maj_size;
    }
    
    // Mise a jour de la tête de lecture après écriture
    fs.opened_file[desc].tete_lecture = lecteur;
    fprintf(fs.log, "\nTête de lecture en fin d'écriture : %d\n", lecteur);

    return maj_size;

}

/**
 * @brief Lit des données à partir d'un fichier ouvert.
 *
 * @param desc Descripteur du fichier ouvert.
 * @param texte Buffer où stocker les données lues.
 * @param size Nombre d'octets à lire.
 */
void read_file(int desc, char *texte, int size){

    // Vérifier si le descripteur est valide
    if (desc > MAX_FILE_OPEN || desc < 0 || fs.opened_file[desc].inode == -1){
        fprintf(fs.log, "\nErreur sur la lecture du fichier de descripteur %d\n", desc);
        printf("Erreur : descrpiteur invalide\n");
    } else if(size < 0) {
        printf("Erreur : taille négatif\n");
    // Vérifier que le fichier n'est pas un répertoire
    } else if(fs.inodes[fs.opened_file[desc].inode].type != 1 && fs.inodes[fs.opened_file[desc].inode].type != 2){
        fprintf(fs.log, "\nErreur sur la lecture du fichier de descripteur %d\n", desc);
        printf("Erreur : le type de fichier est un répertoire ou non reconnu\n");
    } else { 
        //Vérification permission'r'
        int inode = fs.opened_file[desc].inode;
        if (!has_permission(inode, 'r')) {
            fprintf(fs.log, "\nErreur sur la lecture du fichier de descripteur %d\n", desc);
            printf("Erreur : permission de lecture refusée pour cet inode.\n");
            return; // on stoppe la fonction ici
        }

        // tete de lecture et inode du fichier
        int lecteur = fs.opened_file[desc].tete_lecture;

        fprintf(fs.log, "\nTête de lecture en début de lecture : %d\n", lecteur);

        // chercher l'index du bloc du fichier a lire et le numero du bloc dans le file system
        int block_index = -1;
        int i = 0;
        int num_block;
        while (block_index == -1 && i<NUM_BLOCKS){
            // Si la tete de lecture se trouve entre le bloc et le bloc suivant, on recupere le bloc
            num_block = fs.inodes[inode].blocks[i];
            if (sizeof(Filesystem) + num_block*BLOCK_SIZE <= lecteur && lecteur < sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
                block_index = i;
            }
            i++;
        }

        // Verifier si la tete de lecture est dans le bloc
        if(block_index == -1){
            fprintf(fs.log, "\nErreur sur la lecture du fichier de descripteur %d\n", desc);
            printf("Erreur : la tête de lecture n'est pas dans le fichier.\n");
        } else {
            // On copie chaque caractere du file system dans le buffer
            int j = 0;
            while (j<size && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
                fseek(fs.file, lecteur, SEEK_SET);
                fread(texte+j, sizeof(char),1,fs.file);
                j++;
                lecteur++;
            }


            // On réitère le processus jusqu'à la fin de la taille du mot a lire
            int stop = 0;
            while (j<size && !stop){
                block_index++;
                num_block = fs.inodes[inode].blocks[block_index];

                // Vérifier si on est toujours dans le fichier
                if(num_block == -1){
                    stop = 1;
                    fprintf(fs.log, "\nErreur sur la lecture du fichier de descripteur %d\n", desc);
                    printf("Erreur : Fin du fichier dépassé par la tête de lecture\n");
                } else {
                    // On se positionne au bon endroit
                    lecteur = sizeof(Filesystem) + num_block*BLOCK_SIZE;
                    while(j<size && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
                        fseek(fs.file, lecteur, SEEK_SET);
                        fread(texte+j,sizeof(char),1,fs.file);
                        j++;
                        lecteur++;
                    }
                }
            }
        }
        // On marque la fin du texte
        texte[size] = '\0';

        // Mise a jour de la tête de lecture après écriture
        fs.opened_file[desc].tete_lecture = lecteur;
        fprintf(fs.log, "\nTête de lecture en fin de lecture : %d\n", desc);
    }
    


}

/**
 * @brief Ferme un fichier ouvert et libère son descripteur.
 *
 * @param desc Descripteur du fichier à fermer.
 */
void close_file(int desc){
    // Vérifier si le descripteur est valide
    if (desc > MAX_FILE_OPEN || desc < 0 || fs.opened_file[desc].inode == -1){
        printf("Erreur : descrpiteur invalide\n");
        fprintf(fs.log, "\nErreur sur la fermeture du descripteur %d\n", desc);
    } else {
        fs.opened_file[desc].inode = -1;
        fs.opened_file[desc].tete_lecture = -1;
        fprintf(fs.log, "\nDescripteur %d fermé\n", desc);
    }
}


/**
 * @brief Déplace la tête de lecture d'un fichier ouvert.
 *
 * @param desc Descripteur du fichier.
 * @param offset Décalage à appliquer.
 * @param whence Origine du déplacement : 0=début, 1=fin, 2=position actuelle.
 */
void seek_file(int desc, int offset, int whence){
    // Vérifier si le descripteur est valide
    if (desc > MAX_FILE_OPEN || desc < 0 || fs.opened_file[desc].inode == -1){
        fprintf(fs.log, "\nErreur sur le déplacement dans le fichier de descripteur %d\n", desc);
        printf("Erreur : descrpiteur invalide\n");
    // Offset doit etre > 0
    } else if (offset < 0) {
        fprintf(fs.log, "\nErreur sur le déplacement dans le fichier de descripteur %d\n", desc);
        printf("Erreur : offset < 0\n");
    } else {
        // Le cas ou on se poistionne par rapport au debut
        if (whence == 0){
            fprintf(fs.log, "\nDéplacement à partir du début\n");
            // On place le lecteur au debut du fichier
            int inode = fs.opened_file[desc].inode;
            fs.opened_file[desc].tete_lecture = sizeof(Filesystem) + fs.inodes[inode].blocks[0]*BLOCK_SIZE;
            int lecteur = fs.opened_file[desc].tete_lecture;

            fprintf(fs.log, "\nTête de lecture avant le déplacement : %d\n",lecteur);

            // On avance de bloc tant qu'on arrive pas a l'endroit souhaité
            int block_index = 0;
            int j = 0;
            int stop = 0;
            while (j<offset && !stop){
                // On se positionne au bloc suivant
                int num_block = fs.inodes[inode].blocks[block_index];
                if(num_block == -1){
                    stop = 1;
                    fprintf(fs.log, "\nErreur sur le déplacement dans le fichier de descripteur %d\n", desc);
                    printf("Erreur : tete de lecture en dehors du fichier\n");
                } else {
                    // On avance tant qu'on arrive pas a l'endroit souhaité
                    lecteur = sizeof(Filesystem) + num_block*BLOCK_SIZE;
                    while(j<offset && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){ 
                        j++;
                        lecteur++;
                    }
                }
                block_index++;
            }
            // On met a jour la tete de lecture
            fs.opened_file[desc].tete_lecture = lecteur;
            fprintf(fs.log, "\nTête de lecture en fin de déplacement : %d\n", lecteur);
        

        } else {
            fprintf(fs.log, "\nDéplacement par rapport à la position courante\n");
            // Cas où on se positionne par rapport a la position courante
            if (whence == 2){
                int lecteur = fs.opened_file[desc].tete_lecture;
                int inode = fs.opened_file[desc].inode;
                int block_index = -1;
                int i = 0;

                fprintf(fs.log, "\nTête de lecture avant le déplacement : %d\n",lecteur);

                // On cherche l'indice du bloc du fichier et le numéro du bloc dans le file system
                int num_block;
                while (block_index == -1 && i<NUM_BLOCKS){
                    num_block = fs.inodes[inode].blocks[i];
                    if (sizeof(Filesystem) + num_block*BLOCK_SIZE <= lecteur && lecteur < sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){
                        block_index = i;
                    }
                    i++;
                }

                // On avance de offset par rapport a la position couante
                int j = 0;
                int stop = 0;
                while (j<offset && !stop){
                    num_block = fs.inodes[inode].blocks[block_index];
                    // On vérifier si on est dans le fichier
                    if(num_block == -1){
                        stop = 1;
                        fprintf(fs.log, "\nErreur sur le déplacement dans le fichier de descripteur %d\n", desc);
                        printf("Erreur : tete de lecture en dehors du fichier\n");
                    } else {
                        // On positionne la tete de lecture et on avance
                        lecteur = sizeof(Filesystem) + num_block*BLOCK_SIZE;
                        while(j<offset && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){ 
                            j++;
                            lecteur++;
                        }
                    }
                    block_index++;
                }
                // On met a jour la tete de lecture
                fs.opened_file[desc].tete_lecture = lecteur;
                fprintf(fs.log, "\nTête de lecture en fin de déplacement : %d\n", lecteur);
            } else {
                fprintf(fs.log, "\nDéplacement par rapport à la fin\n");
                // Cas où on se positionne par rapport a la fin
                if (whence == 1){
                    // On place le lecteur au debut du fichier
                    int inode = fs.opened_file[desc].inode;
                    fs.opened_file[desc].tete_lecture = sizeof(Filesystem) + fs.inodes[inode].blocks[0]*BLOCK_SIZE;
                    int lecteur = fs.opened_file[desc].tete_lecture;
                    
                    fprintf(fs.log, "\nTête de lecture avant le déplacement : %d\n",lecteur);

                    // On avance de bloc tant qu'on arrive pas a l'endroit souhaité
                    int block_index = 0;
                    int j = 0;
                    int stop = 0;
                    int pos = fs.inodes[inode].size - offset;
                    while (j<pos && !stop){
                        // On se positionne au bloc suivant
                        int num_block = fs.inodes[inode].blocks[block_index];
                        if(num_block == -1){
                            stop = 1;
                            fprintf(fs.log, "\nErreur sur le déplacement dans le fichier de descripteur %d\n", desc);
                            printf("Erreur : tete de lecture en dehors du fichier\n");
                        } else {
                            // On avance tant qu'on arrive pas a l'endroit souhaité
                            lecteur = sizeof(Filesystem) + num_block*BLOCK_SIZE;
                            while(j<pos && lecteur <= sizeof(Filesystem) + (num_block+1)*BLOCK_SIZE){ 
                                j++;
                                lecteur++;
                            }
                        }
                        block_index++;
                    }
                    // On met a jour la tete de lecture
                    fs.opened_file[desc].tete_lecture = lecteur;
                    fprintf(fs.log, "\nTête de lecture en fin de déplacement : %d\n", lecteur);


                } else {
                    printf("Erreur : option non reconnu \n");
                    fprintf(fs.log, "\nOption de déplacement non recconu\n");
                }
            }
        }
    }
}



/**
 * @brief Copie un fichier vers un nouveau fichier dans un répertoire cible.
 *
 * @param filename Nom du fichier à copier.
 * @param newname Nouveau nom du fichier copié.
 * @param inode_dir_source Inode du répertoire source.
 * @param inode_dir_target Inode du répertoire cible.
 * @return L'inode du nouveau fichier ou -1 en cas d'erreur.
 */
int copy_file(char *filename, char *newname, int inode_dir_source, int inode_dir_target) {
    Directory *dir_source = &fs.directories[inode_dir_source];
    Directory *dir_target = &fs.directories[inode_dir_target];

    // Vérifier si le fichier existe
    int source_inode_index = rechInode(filename, *dir_source);
    if(source_inode_index == -1){
        fprintf(fs.log, "\nErreur sur la copie du fichier %s\n", filename);
        printf("Erreur : Fichier inexistant.\n");
        return -1;
    }
    // ----- (1) Vérifier la permission 'r' sur le fichier source -----
    if (!has_permission(source_inode_index, 'r')) {
        printf("Erreur : pas de permission de lecture sur le fichier source '%s'.\n", filename);
        fprintf(fs.log, "\nErreur sur la copie du fichier %s\n", filename);
        return -1;
    }
    // ----- (2) Vérifier la permission 'w' sur le répertoire de destination -----
    if (!has_permission(inode_dir_target, 'w')) {
        printf("Erreur : pas de permission d'écriture dans le répertoire cible.\n");
        fprintf(fs.log, "\nErreur sur la copie du fichier %s\n", filename);
        return -1;
    }

    // Vérifier si un fichier du même nom existe dans le répertoire source
    int exist_target_inode = rechInode(newname, *dir_target);
    if(exist_target_inode != -1){
        printf("Erreur : Un fichier de ce nom existe déjà dans le répertoire.\n");
        fprintf(fs.log, "\nErreur sur la copie du fichier %s\n", filename);
        return -1;
    }

    // Créer un fichier copie
    int new_inode_index = create_file(newname, fs.inodes[source_inode_index].permissions, inode_dir_target);

    // Vérifier si le fichier a été créé
    if (new_inode_index == -1) {
        return -1;
    }
    Inode *source_inode = &fs.inodes[source_inode_index];
    Inode *new_inode = &fs.inodes[new_inode_index];

    
    // Mettre a jour la taille du nouveau fichier
    new_inode->size = source_inode->size;

    

    // Copier les blocs
    char content[new_inode->size];
    int fd1 = open_file(newname, inode_dir_target);
    int fd2 = open_file(filename, inode_dir_source);
    read_file(fd2, content, new_inode->size);
    write_file(fd1, content, new_inode->size);
    close_file(fd1);
    close_file(fd2);

    
    fprintf(fs.log, "\nFichier %s copié vers le répertoire d'inode %d\n", filename, inode_dir_target);
    printf("Fichier %s copié avec succès.\n", filename);
    return new_inode_index;
}



/**
 * Copie récursivement un répertoire (et son contenu) d'un répertoire parent source vers un répertoire parent destination.
 *
 * @param srcDirName   Le nom du répertoire à copier.
 * @param srcParentDir L'inode du répertoire parent source (celui qui contient srcDirName).
 * @param dstParentDir L'inode du répertoire parent de destination (là où on veut copier).
 * @return L'inode du nouveau répertoire copié, ou -1 en cas d'erreur.
 */
 int copy_directory(const char *srcDirName, const char *newname, int srcParentDir, int dstParentDir) {
    // 1) Trouver l'inode du répertoire source
    int srcDirInode = rechInode(srcDirName, fs.directories[srcParentDir]);
    if (srcDirInode == -1) {
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : Le répertoire '%s' n'existe pas dans le répertoire %d.\n", srcDirName, srcParentDir);
        return -1;
    }

    // 2) Vérifier que c'est bien un répertoire
    if (fs.inodes[srcDirInode].type != 0) {  // 0 = répertoire
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : '%s' n'est pas un répertoire.\n", srcDirName);
        return -1;
    }
    // 3) Vérifier la permission 'r' sur le répertoire source
    if (!has_permission(srcDirInode, 'r')) {
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : pas de permission de lecture sur le répertoire source '%s'.\n", srcDirName);
        return -1;
    }



    // 4) Vérifier si un répertoire (ou fichier) du même nom existe déjà dans la destination
    int alreadyInode = rechInode(newname, fs.directories[dstParentDir]);
    if (alreadyInode != -1) {
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : Le nom '%s' existe déjà dans le répertoire de destination.\n", newname);
        return -1;
    }
    // 5) Vérifier la permission 'w' sur le répertoire de destination (pour y créer un nouveau dossier)
    if (!has_permission(dstParentDir, 'w')) {
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : pas de permission d'écriture dans le répertoire destination (inode %d).\n", dstParentDir);
        return -1;
    }

    // 6) Créer un nouveau répertoire dans le répertoire parent de destination
    int newDirInode = create_directory(newname, dstParentDir);
    if (newDirInode == -1) {
        fprintf(fs.log, "\nErreur sur la copie du répertoire %s\n", srcDirName);
        printf("Erreur : Échec de la création du répertoire '%s' dans le répertoire %d.\n", newname, dstParentDir);
        return -1;
    }

    // 7) Parcourir le contenu du répertoire source et copier chaque entrée
    Directory *srcDir = &fs.directories[srcDirInode];
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        int childInode = srcDir->entries[i].inode_index;
        if (childInode != -1) {
            // Récupération des informations de l'enfant
            const char *childName = srcDir->entries[i].filename;
            int childType = fs.inodes[childInode].type;

            if (childType == 1 || childType == 2) {
                // 1 = fichier, 2 = lien symbolique
                // On copie le fichier dans le nouveau répertoire
                copy_file((char *)childName, (char *)childName, srcDirInode, newDirInode);

            } else if (childType == 0) {
                // 0 = répertoire
                // Copie récursive du sous-répertoire
                copy_directory(childName, childName, srcDirInode, newDirInode);
            } 
        }
    }

    fprintf(fs.log, "\nRépertoire '%s' (inode %d) copié dans le répertoire %d (nouveau inode %d)\n", srcDirName, srcDirInode, dstParentDir, newDirInode);
    printf("Répertoire '%s' (inode %d) copié dans le répertoire %d (nouveau inode %d).\n", srcDirName, srcDirInode, dstParentDir, newDirInode);

    return newDirInode;
}



/**
 * @brief Crée un lien dur vers un fichier existant.
 *
 * @param link_name Nom du nouveau lien dur.
 * @param filename Nom du fichier source.
 * @param inode_dir_source Inode du répertoire contenant le fichier source.
 * @param inode_dir_target Inode du répertoire cible où sera créé le lien dur.
 * @return 0 en cas de succès, -1 en cas d'erreur.
 */
int create_hard_link(const char *link_name, char *filename, int inode_dir_source, int inode_dir_target) {
    // Répertoire source et cible
    Directory *dir_source = &fs.directories[inode_dir_source];
    Directory *dir_target = &fs.directories[inode_dir_target];
    int inode_index = rechInode(filename, *dir_source);

    // Vérifier si le fichier existe
    if(inode_index == -1){
        fprintf(fs.log, "\nErreur sur la création de lien dur pour le fichier %s\n", filename);
        printf("Erreur: Fichier inexistant.\n");
        return -1;
    }

    // Vérifier au cas où un fichier du nom du lien existe dans la cible
    int exist_target_inode = rechInode(link_name, *dir_target);
    if(exist_target_inode != -1){
        printf("Erreur : Un fichier de ce nom existe déjà dans le répertoire.\n");
        fprintf(fs.log, "\nErreur sur la création de lien dur pour le fichier %s\n", filename);
        return -1;
    }

    // Chercher une entrée libre dans le répertoire
    int index = rechEntree(inode_dir_target);
    if (index == -1){
        printf("Erreur: Pas d'espace libre dans le répertoire.\n");
        fprintf(fs.log, "\nErreur sur la création de lien dur pour le fichier %s\n", filename);
        return -1;
    }

    // Ajouter le lien dans le répertoire cible
    strncpy(dir_target->entries[index].filename, link_name, MAX_FILE_NAME);
    dir_target->entries[index].inode_index = inode_index;
    fs.inodes[inode_index].link_count++;  // Incrémenter le nombre de liens
    printf("Lien dur '%s' créé pour le fichier '%s'.\n", link_name, filename);
    fprintf(fs.log, "\nLien dur '%s' créé pour le fichier '%s'.\n", link_name, filename);
    return 0;
    
}


/**
 * @brief Déplace un fichier d'un répertoire source vers un répertoire cible.
 *
 * @param filename Nom du fichier à déplacer.
 * @param inode_dir_source Inode du répertoire source.
 * @param inode_dir_target Inode du répertoire cible.
 */
void move_file(char *filename, int inode_dir_source, int inode_dir_target) {
    Directory *dir_source = &fs.directories[inode_dir_source];
    Directory *dir_target = &fs.directories[inode_dir_target];

    // Vérifier si le fichier existe
    int inode_index = rechInode(filename, *dir_source);
    if(inode_index == -1){
        fprintf(fs.log, "\nErreur sur le déplacement du fichier %s\n", filename);
        printf("Erreur: Fichier inexistant.\n");
    } else {
         // Vérifier la permission 'w' sur le répertoire source (on va enlever l'entrée)
        if (!has_permission(inode_dir_source, 'w')) {
            printf("Erreur : pas de permission d'écriture dans le répertoire source (inode %d).\n", inode_dir_source);
            fprintf(fs.log, "\nErreur sur le déplacement du fichier %s\n", filename);
            return;
        }

        // Vérifier si aucun fichier du même nom existe dans le répertoire cible
        int exist_target_inode = rechInode(filename, *dir_target);
        if(exist_target_inode != -1){
            printf("Erreur : Un fichier de ce nom existe déjà dans le répertoire.\n");
            fprintf(fs.log, "\nErreur sur le déplacement du fichier %s\n", filename);
        } else {
            if (!has_permission(inode_dir_target, 'w')) {
                printf("Erreur : pas de permission d'écriture dans le répertoire cible (inode %d).\n", inode_dir_target);
                fprintf(fs.log, "\nErreur sur le déplacement du fichier %s\n", filename);
                return;
            }
        

            Inode *inode = &fs.inodes[inode_index];
            
            // Chercher une entrée libre dans le répertoire
            int index = rechEntree(inode_dir_target);
            if (index == -1){
                printf("Erreur: Pas d'espace libre dans le répertoire cible.\n");
                fprintf(fs.log, "\nErreur sur le déplacement du fichier %s\n", filename);
            } else {

                

                // Ajouter le fichier au répertoire cible
                strncpy(dir_target->entries[index].filename, filename, MAX_FILE_NAME);
                dir_target->entries[index].inode_index = inode_index;

                

                // Supprimer l'entrée du répertoire
                int i = 0;
                int stop = 0;
                while (!stop) {
                    if (dir_source->entries[i].inode_index == inode_index && strcmp(filename, dir_source->entries[i].filename) == 0) {
                        dir_source->entries[i].inode_index = -1;
                        memset(dir_source->entries[i].filename, 0, MAX_FILE_NAME);
                        stop = 1;
                    }
                    i++;
                }
                
                inode->modification_time = time(NULL);  // Mettre à jour le temps de modification

                printf("Fichier déplacé de répertoire %d à répertoire %d.\n", inode_dir_source, inode_dir_target);
                fprintf(fs.log, "\nFichier déplacé de répertoire %d à répertoire %d\n", inode_dir_source, inode_dir_target);
            }
        }
    }
}








/**
 * @brief Sauvegarde l'état du système de fichiers dans un fichier binaire.
 *
 * @param filename Nom du fichier où stocker les données du système de fichiers.
 */
void save_filesystem(const char *filename) {
    // Fermeture de l'instance du filesystem pour éviter les problèmes d'accès concurrents
    fclose(fs.file);

    FILE *file = fopen(filename, "rb+");  // Ouverture en mode binaire écriture
    if (!file) {
        perror("Erreur lors de la sauvegarde du système de fichiers");
        fprintf(fs.log, "\nErreur lors de la sauvegarde du filesystem\n");
        return;
    }

    fwrite(&fs, sizeof(Filesystem), 1, file);  // Écriture de toute la structure
    fclose(file);

    // On ouvre le l'instance du filesystem
    fs.file = fopen(filename, "rb+");
    fprintf(fs.log, "\nSystème de fichier sauvegardé avec succès\n");
}

/**
 * @brief Charge l'état du système de fichiers depuis un fichier binaire.
 *
 * @param filename Nom du fichier contenant la sauvegarde.
 */
void load_filesystem(const char *filename) {
    FILE *file = fopen(filename, "rb");  // Ouverture en mode lecture binaire
    if (!file) {
        printf("Aucune sauvegarde trouvée. Initialisation d'un nouveau système.\n");
        init_filesystem(filename);
    } else {
        fread(&fs, sizeof(Filesystem), 1, file);  // Lecture des données enregistrées
        fclose(file);
        printf("Système de fichiers chargé avec succès.\n");
        fs.file = fopen(filename, "rb+");
        fs.log = fopen("log.txt", "a");   // Création du fichier texte pour les log
        fprintf(fs.log, "\nSystème de fichier chargé avec succès\n");
    }
}

/**
 * @brief Verrouille le système de fichiers pour éviter les accès concurrents.
 */
void lock_filesystem() {
    int fd = fileno(fs.file);  // Obtenir le descripteur de fichier
    flock(fd, LOCK_EX);  // Appliquer un verrou exclusif
}

/**
 * Déverrouille le système de fichiers.
 */
void unlock_filesystem() {
    int fd = fileno(fs.file);
    flock(fd, LOCK_UN);  // Libérer le verrou
}


/**
 * @brief Affiche l'état actuel du système de fichiers (inodes et répertoire courant).
 *
 * @param current_dir Inode du répertoire courant.
 */
void display_filesystem(int current_dir) {
    Directory dir = fs.directories[current_dir];

    printf("\n===== État du système de fichiers =====\n");

    // Affichage des inodes
    printf("Inodes utilisés :\n");
    for (int i = 0; i < NUM_INODES; i++) {
        if (fs.inodes[i].size >= 0) { // Seuls les inodes utilisés sont affichés
            printf("Inode %d: Taille=%d octets, Liens=%d, Permissions=%s\n",
                   fs.inodes[i].id, fs.inodes[i].size, fs.inodes[i].link_count, fs.inodes[i].permissions);
        }
    }

    // Affichage des fichiers dans le répertoire courant
    printf("\nRépertoire courant :\n");
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        if (dir.entries[i].inode_index != -1) {
            printf("- %s (inode %d) (type %d)\n", dir.entries[i].filename, dir.entries[i].inode_index, fs.inodes[dir.entries[i].inode_index].type);
        }
    }
    printf("\n=======================================\n");
}


/**
 * @brief Change le répertoire courant en suivant le chemin spécifié.
 *
 * @param path Chemin absolu ou relatif vers le nouveau répertoire.
 * @param inode_dir Inode du répertoire actuel.
 * @return Inode du nouveau répertoire courant ou l'inode initial en cas d'erreur.
 */
int changerRep(char *path, int inode_dir){

    // On cherche l'inode du chemin
    int inode = get_inode_from_path(path, inode_dir);

    // Vérifier si le chemin est valide
    if (inode == -1){
        fprintf(fs.log, "\nErreur sur le changement de répertoire vers %s\n", path);
        printf("Erreur : chemin non valide\n");
        return inode_dir;
    }

    // Vérifier si on a bien un répertoire
    if (fs.inodes[inode].type != 0){
        fprintf(fs.log, "\nErreur sur le changement de répertoire vers %s\n", path);
        printf("Erreur : le fichier cible du chemin n'est pas un répertoire.\n");
        return inode_dir;
    }

    fprintf(fs.log, "\nNouveau répertoire courant : %s\n", path);
    return inode;
}



/**
 * @brief Affiche le message d'aide décrivant les commandes disponibles.
 */
void print_help() {
    printf("Mini Gestionnaire de Fichiers\n");
    printf("Usage: ./filesystem [OPTIONS]\n\n");

    printf("Options:\n");
    printf("  --help           Affiche ce message d'aide\n");
    printf("  --init           Force une nouvelle initialisation du système de fichiers\n\n");

    printf("Commandes disponibles en mode interactif :\n");
    printf("  cd <path>                        Changer de répertoire\n");
    printf("  chmod <fichier> <perms>          Modifier les permissions (ex: rwx, r--, etc.)\n");
    printf("  cp <src> <newname> <dest_path>   Copier un fichier ou répertoire\n");
    printf("  exit                             Quitter le programme\n");
    printf("  help                             Afficher ce message d'aide\n");
    printf("  ln <filename> <linkname> <path>  Créer un lien dur du fichier filename dans le répertoire path\n");
    printf("  ls                               Lister les fichiers du répertoire courant\n");
    printf("  mkdir <dir>                      Créer un répertoire\n");
    printf("  mv <src> <dest_path>             Déplacer un fichier ou répertoire\n");
    printf("  pwd                              Afficher le répertoire courant\n");
    printf("  remdir <dir>                     Supprimer un répertoire récursivement\n");
    printf("  rm <file>                        Supprimer un fichier\n");
    printf("  rfile <filename>                 Afficher le contenu d'un fichier\n");
    printf("  stat <file>                      Afficher les informations d'un fichier ou répertoire\n");
    printf("  sym <target_path> <linkname>     Créer un lien symbolique vers le fichier dans path\n");
    printf("  touch <file>                     Créer un fichier vide\n");
    printf("  wfile <filename> <mode> <texte>  Écrire dans un fichier (modes: add, rewrite)\n");
}


/**
 * @brief Affiche le prompt avec le chemin complet du répertoire courant.
 *
 * @param current_dir Inode du répertoire courant.
 */
void print_prompt(int current_dir) {
    // Augmenter la taille du buffer pour le chemin
    char path[2048] = "";  // Augmenté de 1024 à 2048
    int dir = current_dir;
    
    printf("fs:/");

    while (dir != 0 && dir != -1) {
        char dirname[MAX_FILE_NAME] = "?";
        int parent = fs.inodes[dir].inode_rep_parent;
        
        // Vérifier que le parent est valide avant d'y accéder
        if (parent < 0 || parent >= NUM_DIRECTORY_ENTRIES) {
            break;
        }

        Directory *parent_dir = &fs.directories[parent];

        for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
            if (parent_dir->entries[i].inode_index == dir) {
                strncpy(dirname, parent_dir->entries[i].filename, MAX_FILE_NAME - 1);
                dirname[MAX_FILE_NAME - 1] = '\0'; // Assurer la terminaison
                break;
            }
        }

        // Vérifier si nous avons assez d'espace avant de concaténer
        if (strlen(dirname) + strlen(path) + 2 < sizeof(path)) {  // +2 pour '/' et '\0'
            char temp[2048] = "/";
            strncat(temp, dirname, sizeof(temp) - strlen(temp) - 1);
            strncat(temp, path, sizeof(temp) - strlen(temp) - 1);
            strncpy(path, temp, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';  // Assurer la terminaison
        } else {
            printf("[chemin trop long]");
            break;
        }

        dir = parent;
    }
    
    printf("%s> ", path);
    fflush(stdout);
}


/**
 * @brief Génère le chemin complet à partir de l'inode du répertoire courant.
 *
 * @param current_dir Inode du répertoire courant.
 * @param path Buffer où stocker le chemin généré.
 * @param path_size Taille du buffer fourni.
 */
void generate_full_path(int current_dir, char *path, size_t path_size) {
    if (!path || path_size == 0) return; // Vérifier les paramètres

    path[0] = '\0';  // Initialiser comme une chaîne vide
    int dir = current_dir;

    while (dir != 0 && dir != -1) {
        char dirname[MAX_FILE_NAME] = "?";
        int parent = fs.inodes[dir].inode_rep_parent;

        // Vérifier que l'index du parent est valide
        if (parent < 0 || parent >= NUM_DIRECTORY_ENTRIES) break;

        Directory *parent_dir = &fs.directories[parent];

        for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
            if (parent_dir->entries[i].inode_index == dir) {
                strncpy(dirname, parent_dir->entries[i].filename, MAX_FILE_NAME - 1);
                dirname[MAX_FILE_NAME - 1] = '\0';  // S'assurer de la terminaison
                break;
            }
        }

        // Vérifier si on a assez de place avant de concaténer
        if (strlen(path) + strlen(dirname) + 2 < path_size) {
            char temp[2048];
            snprintf(temp, sizeof(temp), "/%s%s", dirname, path);
            strncpy(path, temp, path_size - 1);
            path[path_size - 1] = '\0';  // Assurer la terminaison
        } else {
            strncpy(path, "[chemin trop long]", path_size - 1);
            path[path_size - 1] = '\0';
            break;
        }

        dir = parent;
    }
}

/**
 * @brief Affiche les descripteurs de fichiers actuellement ouverts.
 */
void print_desc(){
    for (int i = 0; i < MAX_FILE_OPEN ; i++){
        if (fs.opened_file[i].inode != -1){
            printf("Descripteur %d : inode %d\n", i, fs.opened_file[i].inode);
        }
    }
}


/**
 * @brief Liste le contenu du répertoire courant avec détails (permissions, type, taille).
 *
 * @param current_dir Inode du répertoire courant.
 */
void list_directory(int current_dir) {
    Directory dir = fs.directories[current_dir];
    printf("Contenu du répertoire :\n");
    
    for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++) {
        if (dir.entries[i].inode_index != -1) {
            Inode *inode = &fs.inodes[dir.entries[i].inode_index];
            char type = '?';
            char perm[4] = "---";
            
            switch(inode->type) {
                case 0: type = 'd'; break;  // Répertoire
                case 1: type = 'f'; break;  // Fichier
                case 2: type = 'l'; break;  // Lien symbolique
            }
            
            // Afficher les permissions de manière lisible
            if (inode->permissions[0] == 'r') perm[0] = 'r';
            if (inode->permissions[1] == 'w') perm[1] = 'w';
            if (inode->permissions[2] == 'x') perm[2] = 'x';
            
            printf("[%c%s] %-20s (inode %d, taille %d octets)\n", 
                   type, perm, dir.entries[i].filename, 
                   dir.entries[i].inode_index, inode->size);
        }
    }
}

/**
 * @brief Affiche les informations détaillées sur un fichier ou répertoire.
 *
 * @param filename Nom du fichier ou répertoire à examiner.
 * @param current_dir Inode du répertoire courant.
 */
void print_file_info(const char *filename, int current_dir) {
    int inode = rechInode(filename, fs.directories[current_dir]);
    if (inode == -1) {
        printf("Fichier '%s' introuvable\n", filename);
        return;
    }
    
    Inode *node = &fs.inodes[inode];
    printf("Informations sur '%s':\n", filename);
    printf("  Inode: %d\n", inode);
    printf("  Type: %s\n", 
           node->type == 0 ? "Répertoire" : 
           node->type == 1 ? "Fichier" : 
           node->type == 2 ? "Lien symbolique" : "Inconnu");
    printf("  Taille: %d octets\n", node->size);
    printf("  Permissions: %s\n", node->permissions);
    printf("  Liens: %d\n", node->link_count);
    printf("  Créé le: %s", ctime(&node->creation_time));
    printf("  Modifié le: %s", ctime(&node->modification_time));
}



/**
 * @brief Lance le shell interactif du gestionnaire de fichiers.
 *
 * @param force_init Force la réinitialisation du système de fichiers si non nul.
 * @return Code de sortie du shell interactif.
 */
int interactive_shell(int force_init) {
    if (force_init) {
        printf("Initialisation forcée du système de fichiers...\n");
        init_filesystem("filesystem.img");
        
        // Créer une structure de répertoires de base
        create_directory("usr", 0);
        int home_dir = create_directory("home", 0);
        create_directory("local", rechInode("usr", fs.directories[0]));
        fs.current_dir = home_dir; // Démarrer dans /home
    } else {
        load_filesystem("filesystem.img");
    }

    save_filesystem("filesystem.img");

    int current_dir = fs.current_dir;
    char command[256];
    char arg1[256];
    char arg2[256];
    char arg3[256];
    
    printf("Mini Gestionnaire de Fichiers. Tapez 'help' pour l'aide.\n");
    
    while (1) {
        print_prompt(current_dir);
        
        if (fgets(command, sizeof(command), stdin) != NULL) {
            // Supprimer le saut de ligne
            command[strcspn(command, "\n")] = 0;
            
            if (strcmp(command, "exit") == 0) {
                fprintf(fs.log, "\n\n\ncommande effectué : exit\n");
                break;

            } else if (strcmp(command, "help") == 0) {
                fprintf(fs.log, "\n\n\ncommande effectué : help\n");
                print_help();
                save_filesystem("filesystem.img");


            } else if (strcmp(command, "ls") == 0) {
                fprintf(fs.log, "\n\n\ncommande effectué : ls\n");
                list_directory(current_dir);
                save_filesystem("filesystem.img");


            } else if (strcmp(command, "pwd") == 0) {
                fprintf(fs.log, "\n\n\ncommande effectué : pwd\n");
                char path[2048] = "";
                generate_full_path(current_dir, path, sizeof(path));

                // Vérifier si le chemin commence par '/' et éviter une double barre
                printf("/%s\n", path[0] == '/' ? path + 1 : path);
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "cd %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : cd %s\n", arg1);
                int new_dir = changerRep(arg1, current_dir);
                // Vérifier si on a pu accéder a un nouveau répertoire
                if (new_dir != -1) {
                    current_dir = new_dir;
                    fs.current_dir = current_dir;
                } else {
                    printf("Erreur: chemin invalide ou ce n'est pas un répertoire\n");
                }
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "mkdir %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : mkdir %s\n", arg1);
                if (create_directory(arg1, current_dir) == -1) {
                    printf("Erreur: impossible de créer le répertoire\n");
                }
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "touch %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : touch %s\n", arg1);
                if (create_file(arg1, "rw-", current_dir) == -1) {
                    printf("Erreur: impossible de créer le fichier\n");
                }
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "rm %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : rm %s\n", arg1);
                delete_file(arg1, current_dir);
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "remdir %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : remdir %s\n", arg1);
                if (delete_directory(arg1, current_dir) == -1) {
                    printf("Erreur: impossible de supprimer le répertoire\n");
                }
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "cp %s %s %s", arg1, arg2, arg3) == 3) {
                fprintf(fs.log, "\n\n\ncommande effectué : cp %s %s %s\n", arg1, arg2, arg3);
                int inode = get_inode_from_path(arg3, current_dir);
                // Vérifier si le chemin est bien valide et est un répertoire
                if (inode == -1 || fs.inodes[inode].type != 0){
                    printf("Erreur : répertoire cible invalide.\n");
                } else {
                    // Vérifier l'existence du fichier a copier
                    int inode_src = rechInode(arg1, fs.directories[current_dir]);
                    if (inode_src == -1){
                        printf("Erreur : fichier non existant \n");
                    } else {
                        // Vérifier le type de fichier pour choisir la fonction coper a effectuer
                        if (fs.inodes[inode_src].type == 1 || fs.inodes[inode_src].type == 2){
                            if (copy_file(arg1, arg2, current_dir, inode) == -1) {
                                printf("Erreur lors de la copie\n");
                            }
                        } else if (fs.inodes[inode_src].type == 0) {
                            if (copy_directory(arg1, arg2, current_dir, inode) == -1){
                                printf("Erreur lors de la copie\n"); 
                            }
                        } else {
                            printf("Erreur : type de fichier non reconnu\n");
                        }
                    }
                }
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "mv %s %s", arg1, arg2) == 2) {
                fprintf(fs.log, "\n\n\ncommande effectué : mv %s %s\n", arg1, arg2);
                int src_inode = rechInode(arg1, fs.directories[current_dir]);
                // Vérifier la validité de l'inode
                if (src_inode == -1) {
                    printf("Erreur: fichier source introuvable\n");
                    continue;
                }
                int dest_dir = get_inode_from_path(arg2, current_dir);
                // Vérifier que le chemin est un répertoire
                if (dest_dir != -1 && fs.inodes[dest_dir].type == 0) {
                    // Vérifier le typre de fichier a deplacer
                    if (fs.inodes[src_inode].type == 0){
                        move_directory(arg1, current_dir, dest_dir);
                    } else if (fs.inodes[src_inode].type == 1 || fs.inodes[src_inode].type == 2){
                        move_file(arg1, current_dir, dest_dir);
                    }else {
                        printf("Erreur : type de fichier non reconnu\n");
                    }
                } else {
                    printf("Erreur : répertoire cible invalide.\n");
                }
                save_filesystem("filesystem.img");
                

            } else if (sscanf(command, "ln %s %s %s", arg1, arg2, arg3) == 3) {
                fprintf(fs.log, "\n\n\ncommande effectué : ln %s %s %s\n", arg1, arg2, arg3);
                int inode = get_inode_from_path(arg3, current_dir);
                // Vérifier que le chemin mène a un répertoire
                if (inode == -1 || fs.inodes[inode].type != 0){
                    printf("Erreur : répertoire cible invalide.\n");
                } else {
                    if (create_hard_link(arg2, arg1, current_dir, inode) == -1) {
                        printf("Erreur lors de la création du lien dur\n");
                    }
                }
                save_filesystem("filesystem.img");
            } else if (sscanf(command, "sym %s %s", arg1, arg2) == 2) {
                if (create_symbolic_link(arg2, arg1, current_dir) == -1) {
                    printf("Erreur lors de la création du lien symbolique\n");
                }
                save_filesystem("filesystem.img");
           
                
            } else if (sscanf(command, "rfile %s", arg1) == 1){
                fprintf(fs.log, "\n\n\ncommande effectué : rfile %s\n", arg1);
                int inode = rechInode(arg1, fs.directories[current_dir]);
                // Vérifier que le fichier existe
                if(inode == -1){
                    printf("Erreur : fichier non existant\n");
                } else {
                    // Vérifier si on a un lien ou un fichier
                    if(fs.inodes[inode].type == 1){
                        // Ouvrir le fichier puis lire puis fermer puis afficher
                        int fd = open_file(arg1, current_dir);
                        int size = fs.inodes[fs.opened_file[fd].inode].size;
                        seek_file(fd, 0, 0);
                        char texte[size+1];
                        read_file(fd, texte, size);
                        printf("contenu du fichier : %s\n", texte);
                        save_filesystem("filesystem.img");

                    } else if(fs.inodes[inode].type == 2){
                        // Lecture du lien symbolique pour retrouver le fichier a llire
                        char path[fs.inodes[inode].size + 1];
                        int fd = open_file(arg1, current_dir);
                        seek_file(fd, 0, 0);
                        read_file(fd, path, fs.inodes[inode].size);
                        close(fd);
                        //printf("path : %s\n", path);

                        // Chercher le fichier a lire
                        int inode_target = get_inode_from_path(path, current_dir);
                        int rep_parent = fs.inodes[inode_target].inode_rep_parent;
                        int stop = 0;
                        char filename[MAX_FILE_NAME];
                        int i = 0;
                        while(i<NUM_DIRECTORY_ENTRIES && !stop){
                            if(fs.directories[rep_parent].entries[i].inode_index == inode_target){
                                strncpy(filename, fs.directories[rep_parent].entries[i].filename, strlen(fs.directories[rep_parent].entries[i].filename));
                                stop = 1;
                            }
                            i++;
                        }
                        filename[strlen(fs.directories[rep_parent].entries[i-1].filename)] = '\0';

                        // Ouverture + Lecture du fichier lu
                        fd = open_file(filename, rep_parent);
                        int size = fs.inodes[fs.opened_file[fd].inode].size;
                        seek_file(fd, 0, 0);
                        char texte[size+1];
                        read_file(fd, texte, size);
                        printf("contenu du fichier : %s\n", texte);
                        save_filesystem("filesystem.img");
                        
                    } else if(fs.inodes[inode].type == 0) {
                        printf("Erreur : tentation de lecture d'un répertoire\n");
                    } else {
                        printf("Erreur : type de fichier non reconnu\n");
                    }
                }
                

            } else if (sscanf(command, "wfile %s %s %[^\n]", arg1, arg3, arg2) == 3){
                fprintf(fs.log, "\n\n\ncommande effectué : wfile %s %s %s\n", arg1, arg3, arg2);
                // Vérification si on ajoute ou on ecrase le contenu
                if(strcmp(arg3, "add") == 0){
                    int fd = open_file(arg1, current_dir);
                    int size = strlen(arg2);
                    seek_file(fd, 0, 1);
                    write_file(fd, arg2, size);
                    close_file(fd);
                } else if (strcmp(arg3, "rewrite") == 0){
                    int fd = open_file(arg1, current_dir);
                    int size = strlen(arg2);
                    seek_file(fd, 0, 0);
                    write_file(fd, arg2, size);
                    close_file(fd);
                } else {
                    printf("mode d'écriture non reconnu\n");
                }
                save_filesystem("filesystem.img");
            
                
            } else if (sscanf(command, "stat %s", arg1) == 1) {
                fprintf(fs.log, "\n\n\ncommande effectué : stat %s\n", arg1);
                print_file_info(arg1, current_dir);
                save_filesystem("filesystem.img");


            } else if (sscanf(command, "chmod %s %s", arg1, arg2) == 2) {
                fprintf(fs.log, "\n\n\ncommande effectué : chmod %s %s\n", arg1, arg2);
                // arg1 = nom du fichier/répertoire, arg2 = nouvelles permissions
                if (change_permissions(arg1, arg2, current_dir) == -1) {
                    printf("Erreur : impossible de modifier les permissions.\n");
                }
                save_filesystem("filesystem.img");

            
            } else if (strlen(command) > 0) {
                fprintf(fs.log, "\n\n\ncommande effectué : %s\n", command);
                printf("Commande inconnue: %s\n", command);
                save_filesystem("filesystem.img");
            }
            
        }
    }
    
    fprintf(fs.log, "\n\nFermeture du système de fichier\n");
    save_filesystem("filesystem.img");
    fclose(fs.log);
    fclose(fs.file);
    return 0;
}






int main(int argc, char *argv[]) {
    int force_init = 0;
    int opt;
    
    // Analyse des arguments en ligne de commande
    while ((opt = getopt(argc, argv, "hi")) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'i':
                force_init = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-h] [-i]\n", argv[0]);
                return 1;
        }
    }
    
    // Démarrer le shell interactif
    return interactive_shell(force_init);
}