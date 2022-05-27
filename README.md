Ce projet est compilé à l'aide d'un Makefile.
Ci-dessous les règles disponibles.

Pour compiler le projet :

```bash
make install
```
L'arborescence, les executables (avec notre bibliothèque) ainsi que notre bibliothèque sont créés.
Les executables de nos propres tests sont également construit dans install/test.

Pour exécuter l'ensemble des tests :
```bash
make check
```

Pour exécuter l'ensemble des tests sous valgrind :
```bash
make check && make valgrind
```

Pour génerer et enregistrer les graphs :
```bash
make check && make pthreads && make graphs
```
La création des graphs est assez longue, environ 7 minutes au total.

Pour supprimer le répertoire install/ :
```bash
make check && make pthreads && make graphs
```
Attention les graphs seront également supprimés.