import os
import sys

def main(argv):

    orig = argv[0]
    dest = argv[1]

    cmd_str = "scp s103061527@140.114.91.171:" + orig + " " + dest
    os.system( cmd_str )


if __name__ == '__main__': main(sys.argv[1:])
