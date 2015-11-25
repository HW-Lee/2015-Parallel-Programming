import os
import sys

def main(argv):
    
    usr = "user16"

    opt = ""
    if len(argv) > 1: opt = argv[1]

    cmd = "ssh " + opt + " " + usr + "@" + "140.114.91.176"
    os.system( cmd )


if __name__ == '__main__': main(sys.argv)