import os
import sys
import random

def main(argv):
    casesize = int(argv[0])

    outfile = file("test" + str(casesize), "w")
    outfile.write(str(casesize) + " " + str(casesize * 5) + "\n")

    for x in range(casesize * 5):
        pair = random.sample(range(1, casesize+1), 2)
        dist = random.randint(1, 100)
        if pair[0] == pair[1]: dist = 0
        outfile.write(reduce(lambda x, y: str(x) + " " + str(y), pair) + " " + str(dist) + "\n")

    outfile.write("\n")

if __name__ == "__main__": main(sys.argv[1:])
