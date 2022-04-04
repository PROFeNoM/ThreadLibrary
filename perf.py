
# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt


args = sys.argv[1:]

if (len(args) < 2):
    print("Erreur - Besoin du nom du 1er fichier et le nombre de threads")
    exit()

def getTime(fileName):
    if len(args) == 3:
        subprocess.run(["./" + fileName, str(nb_threads), str(nb_yields)])
    elif len(args) == 2:
        subprocess.run(["./" + fileName, str(nb_threads)])

    f = open("time.txt", "r")
    timeString = f.read()
    times = timeString.split("\n")
    f.close()

    return float(times[0])

time1 = []
time2 = []

filename1 = args[0]
filename2 = filename1 + "_c"

nb_threads = int(args[1])
if len(args) == 3:
    nb_yields = int(args[2])
step = 10

for i in range(1, nb_threads, step):
    time1.append(getTime(filename1))
    time2.append(getTime(filename2))


RangeThreads = range(1, nb_threads, step)


plt.plot(RangeThreads, time1, '-r')
plt.plot(RangeThreads, time2, '-b')
plt.ylabel("Temps en secondes")
plt.xlabel("Nombre de threads")
plt.title("Performances de notre librairie et de la librairie pthread")
plt.show()
