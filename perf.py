
# Import system modules
import subprocess
import matplotlib.pyplot as plt


def getTime(fileName):
    subprocess.run(fileName)

    f = open("time.txt", "r")
    timeString = f.read()
    times = timeString.split("\n")
    f.close()

    return float(times[0])

time1 = []
time2 = []


for i in range(1, 11):
    time1.append(getTime("./test"))
    time2.append(getTime("./test"))


nbThreads = range(1, 11)


plt.plot(nbThreads, time1, '-r')
plt.plot(nbThreads, time2, '-b')
plt.ylabel("Temps en secondes")
plt.xlabel("Nombre de threads")
plt.title("Performances de notre librairie et de la librairie pthread")
plt.show()
