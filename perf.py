
# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt
import re

args = sys.argv[1:]

if (len(args) < 2):
    print("Erreur - Besoin du nom du 1er fichier et le nombre de threads")
    exit()

def parser(result):
    spliter = re.split(r" |\n", result.stdout.decode("utf-8"))

    for i in range(0, len(spliter)):
        if spliter[i] == "s" or spliter[i] == "us":
            return tuple([int(spliter[i-1]), spliter[i]])

    return (0, "s")

def getTime(fileName):
    if len(args) == 3:
        result = subprocess.run(["./" + fileName, str(nb_threads), str(nb_yields)], stdout = subprocess.PIPE)
    elif len(args) == 2:
        result = subprocess.run(["./" + fileName, str(nb_threads)], stdout = subprocess.PIPE)


    (time, unit) = parser(result)

    if unit == "us":
        return time/1000
    elif unit == "s":
        return time*1000

    return time

time1 = []
time2 = []

filename1 = args[0]
filename2 = filename1 + "_c"

nb_threads = int(args[1])
if len(args) == 3:
    nb_yields = int(args[2])
step = int(nb_threads/10)

for i in range(1, nb_threads, step):
    time1.append(getTime(filename1))
    time2.append(getTime(filename2))


RangeThreads = range(1, nb_threads, step)


plt.plot(RangeThreads, time1, '-r', label='notre bib')
plt.plot(RangeThreads, time2, '-b', label='pthread bib')
plt.ylabel("Temps en ms")
plt.xlabel("Nombre de threads")
plt.title("Performances de notre bibliothèque et de la bibliothèque pthread")
plt.legend()
# plt.show()
name = filename1.split("/")
plt.savefig('install/graphs/' + name[2])
