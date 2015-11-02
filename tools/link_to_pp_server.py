import os
import sys

def main(argv):
    
    usr = "s103061527"

    opt = ""
    if len(argv) > 1: opt = argv[1]

    cmd = "ssh " + opt + " " + usr + "@" + "140.114.91.171"
    os.system( cmd )


if __name__ == '__main__': main(sys.argv)