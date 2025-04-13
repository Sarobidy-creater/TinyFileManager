# Règle principale
all: filesystem

# Génération de l'exécutable
filesystem:
	gcc -o filesystem filesystem.c

# Nettoyer les fichiers compilés
clean:
	rm -f filesystem
