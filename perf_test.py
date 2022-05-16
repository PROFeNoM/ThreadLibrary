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
    result = subprocess.run(["./" + fileName, str(size_array)], stdout = subprocess.PIPE)


    time = parser(result)

    return time

time1 = []
time2 = []

filename1 = args[0]
filename2 = filename1 + "_c"

size_array = int(args[1])

if len(args) == 3:
    nb_yields = int(args[2])
step = int(size_array/50)

bar = Bar.Bar('Running ' + filename1 + ' vs. ' + filename2, step=step, max=size_array / step, suffix='%(percent).1f%% - %(eta)ds')
for i in range(1, size_array, step):
    # print(f'Running {filename1} & {filename2} ==> {i}.' + '\r')

    time1.append(getTime(filename1, i))
    time2.append(getTime(filename2, i))
    bar.next()
bar.finish()

RangeThreads = range(1, size_array, step)


plt.plot(RangeThreads, time1, '-r', label='notre bib')
plt.plot(RangeThreads, time2, '-b', label='pthread bib')
plt.ylabel("Temps en ms")
plt.xlabel("Taille du tableau")
plt.title("Performances de notre bibliothèque et de la bibliothèque pthread")
plt.legend()
# plt.show()
name = filename1.split("/")
plt.savefig('install/graphs/' + name[2])
