import sys
for line in sys.stdin:
    t = line.strip().split()
    print t[0] + " " + t[1]
    print t[1] + " " + t[0]
