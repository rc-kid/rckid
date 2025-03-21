import sys

#iN = 160
#oN = 267

#iN = 144
#oN = 240


# python3 nearest-neighbor.py 144 240 > ../gbcemu/rowScaling.inc.h
# python3 nearest-neighbor.py 160 267 > ../gbcemu/colScaling.inc.h

iN = int(sys.argv[1])
oN = int(sys.argv[2])

result = [0] * iN
for i in range(oN):
    index = i / (oN / iN)
    index = round(index)
    result[index] += 1

print(f"// nearest neighbor scaling from {iN} to {oN}")
for i in result:
    print(f"{i},", end = "")
print("")

