# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt
import re
# import progress.bar as Bar

args = sys.argv[1:]

if (len(args) < 2):
    print("Erreur - Besoin du nom du 1er fichier et la taille du tableau")
    exit()

def parser(result):
    spliter = re.split(r" |\n", result.stdout.decode("utf-8"))

    for i in range(0, len(spliter)):
        if spliter[i] == "s" or spliter[i] == "us":
            return tuple([int(spliter[i-1]), spliter[i]])

    return (0, "s")

def getTime(fileName, size_array):
    result = subprocess.run(["./" + fileName, str(size_array)], stdout = subprocess.PIPE)


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

size_array = int(args[1])
if size_array < 100:
    print("Cannot have less than 100 threads")
    raise ValueError()

if len(args) == 3:
    nb_yields = int(args[2])
step = int(size_array/100)

# bar = Bar.Bar('Running ' + filename1 + ' vs. ' + filename2, step=step, max=size_array / step, suffix='%(percent).1f%% - %(eta)ds')
for i in range(1, size_array, step):
    #print(f'Running {filename1} & {filename2} ==> {i}.' + '\r')

    time1.append(getTime(filename1, i))
    time2.append(getTime(filename2, i))
    # bar.next()
# bar.finish()

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