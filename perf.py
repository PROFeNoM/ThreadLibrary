
"""
perfs (s) haut / param (nb threads, nb yield, param n pour fibo)
2 courbes (thread et pthread)

"""

# Import system modules
import subprocess
from time import time

# Run 1
subprocess.run("./test")
f = open("time.txt", "r");
time1 = str(print(f.read()))
print("Time 1 " + time1)
time1 = float(time1)
print("Time 1 " + time1)

# Run 2
subprocess.run("./test")
f = open("time.txt", "r");
time2 = str(print(f.read()))
time2 = float(time2)
print("Time 1 " + str(time2))
