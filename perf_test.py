# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt
import re
import progress.bar as Bar

args = sys.argv[1:]

if (len(args) < 2):
    print("Erreur - Besoin du nom du 1er fichier et la taille du tableau")
    exit()

def parser(result):
    spliter = re.split(r" |\n", result.stdout.decode("utf-8"))

    for i in range(0, len(spliter)):
        if spliter[i] == "TIME:":
            return int(spliter[i+1])

    return 0

def getTime(fileName, size_array):
    # result = subprocess.run(["LD_LIBRARY_PATH=./install/lib/ python38 perf_test.py " + fileName + " " + str(size_array)], stdout = subprocess.PIPE)
    result1 = subprocess.run(["./" + fileName, str(size_array)], stdout = subprocess.PIPE)
    result2 = subprocess.run(["./" + fileName, str(size_array)], stdout = subprocess.PIPE)
    result3 = subprocess.run(["./" + fileName, str(size_array)], stdout = subprocess.PIPE)

    time1 = parser(result1)
    time2 = parser(result2)
    time3 = parser(result3)


    time = (time1 + time2 + time3) / 3

    return time

time1 = []
time2 = []

filename1 = args[0]
filename2 = filename1 + "_c"

size_array = int(args[1])

if filename1 == "install/test/test_sorting_merge":
    step = 1

if filename1 == "install/test/test_sum":
    step = int(size_array / 200)

bar = Bar.Bar('Running ' + filename1 + ' vs. ' + filename2, step=step, max=size_array / step, suffix='%(percent).1f%% - %(eta)ds')
for i in range(1, size_array, step):
    time1.append(getTime(filename1, i))
    time2.append(getTime(filename2, i))
    bar.next()
bar.finish()

RangeThreads = range(1, size_array, step)


titles = {
    "install/test/test_sum" : "Performances de notre bibliothèque et de la bibliothèque pthread\nSomme des éléments d'un tableau de taille grandissante",
    "install/test/test_sorting_merge" : "Performances de notre bibliothèque et de la bibliothèque pthread\nTri fusion d'un tableau de taille grandissante",
    "install/test/stack_oveflow" : "Performances de notre bibliothèque et de la bibliothèque pthread\nDépassement de pile"
}

final_time1 = time1[len(time1) - 1]
final_time2 = time2[len(time2) - 1]
unit = "us"

if final_time2 > 100000:
    final_time2 = final_time2 / 1000
    unit = "ms"
    time1 = [time / 1000 for time in time1]
    time2 = [time / 1000 for time in time2]


plt.plot(RangeThreads, time1, '-r', label='notre bib')
plt.plot(RangeThreads, time2, '-b', label='pthread bib')
plt.ylabel("Temps en " + unit)
plt.xlabel("Taille du tableau")
plt.title(titles[filename1])
plt.legend()
# plt.show()
name = filename1.split("/")
plt.savefig('install/graphs/' + name[2])
