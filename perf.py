# Import system modules
import subprocess
import sys
import matplotlib.pyplot as plt
import re
import progress.bar as Bar

args = sys.argv[1:]

if (len(args) < 1):
    print("Erreur - Besoin du nom du 1er fichier")
    exit()

def parser(result):
    spliter = re.split(r" |\n", result.stdout.decode("utf-8"))

    for i in range(0, len(spliter)):
        if spliter[i] == "s" or spliter[i] == "us":
            return tuple([int(spliter[i-1]), spliter[i]])

    return (0, "s")

def getTime(fileName, threads_used):
    onlyName = fileName.split("/")[2]
    if ((onlyName == "31-switch-many") or (onlyName == "32-switch-many-join") or (onlyName == "33-switch-many-cascade")):
        result = subprocess.run(["./" + fileName, str(threads_used), str(nb_yields)], stdout = subprocess.PIPE)
    else:
        result = subprocess.run(["./" + fileName, str(threads_used)], stdout = subprocess.PIPE)
    # if len(args) == 3:
    #     result = subprocess.run(["./" + fileName, str(threads_used), str(nb_yields)], stdout = subprocess.PIPE)
    # elif len(args) == 2:
    #     result = subprocess.run(["./" + fileName, str(threads_used)], stdout = subprocess.PIPE)


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

#######################################################################################
# nb_threads = int(args[1])

# if len(args) == 3:
#     nb_yields = int(args[2])

# if ((filename1 == "install/bin/61-mutex") or (filename1 == "install/bin/62-mutex")):
#     step = nb_threads
# elif (filename1 == "install/bin/33-switch-many-cascade"):
#     print("HERE install/bin/33-switch-many-cascade HERE")
#     nb_threads = 20
#     step = 1
# else:
#     step = int(nb_threads/100)
#######################################################################################

name = filename1.split("/")[2]
if ((name == "21-create-many") or (name == "22-create-many-recursive") or (name == "23-create-many-once")):
    nb_threads = 10000
    step = int(nb_threads/100)
elif ((name == "61-mutex") or (name == "62-mutex")):
    nb_threads = 20
    step = 2
elif ((name == "31-switch-many") or (name == "32-switch-many-join")):
    nb_threads = 10000
    nb_yields = 10
    step = int(nb_threads/100)
elif (name == "33-switch-many-cascade"):
    nb_threads = 1000
    nb_yields = 10
    step = int(nb_threads/100)
else:
    nb_threads = 10000
    step = int(nb_threads/100)


bar = Bar.Bar('Running ' + filename1.split("/")[2] + ' vs. ' + filename2.split("/")[2], step=step, max=nb_threads / step, suffix='%(percent).1f%% - %(eta)ds' + ' ')
for i in range(1, nb_threads, step):
    print(f'Running {filename1.split("/")[2]} & {filename2.split("/")[2]} ==> {i}.' + '\r')
    
    time1.append(getTime(filename1, i))
    time2.append(getTime(filename2, i))
    bar.next()
bar.finish()

RangeThreads = range(1, nb_threads, step)

titles = {
    "install/bin/21-create-many" : "Performances de notre bibliothèque et de la bibliothèque pthread\n21-create-many",
    "install/bin/22-create-many-recursive" : "Performances de notre bibliothèque et de la bibliothèque pthread\n22-create-many-recursive",
    "install/bin/23-create-many-once" : "Performances de notre bibliothèque et de la bibliothèque pthread\n23-create-many-once",
    "install/bin/61-mutex" : "Performances de notre bibliothèque et de la bibliothèque pthread\n61-mutex",
    "install/bin/62-mutex" : "Performances de notre bibliothèque et de la bibliothèque pthread\n62-mutex",
    "install/bin/31-switch-many" : "Performances de notre bibliothèque et de la bibliothèque pthread\n31-switch-many",
    "install/bin/32-switch-many-join" : "Performances de notre bibliothèque et de la bibliothèque pthread\n32-switch-many-join",
    "install/bin/33-switch-many-cascade" : "Performances de notre bibliothèque et de la bibliothèque pthread\n33-switch-many-cascade"
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
plt.xlabel("Nombre de threads")
plt.title(titles[filename1])
plt.legend()
# plt.show()
plt.savefig('install/graphs/' + name)
