
# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt


args = sys.argv[1:]

if (len(args) < 1):
    print("Erreur - Besoin du nom du fichier tester")
    exit()

def getTime(fileName):
    subprocess.run(["./" + filename1, "a"])

    f = open("time.txt", "r")
    timeString = f.read()
    times = timeString.split("\n")
    f.close()

    return float(times[0])

time1 = []
time2 = []

filename1 = args[0]

nb_threads = 5
for i in range(1, nb_threads):
    time1.append(getTime(filename1))
    time2.append(getTime(filename1))


RangeThreads = range(1, nb_threads)


plt.plot(RangeThreads, time1, '-r')
plt.plot(RangeThreads, time2, '-b')
plt.ylabel("Temps en secondes")
plt.xlabel("Nombre de threads")
plt.title("Performances de notre librairie et de la librairie pthread")
plt.show()
