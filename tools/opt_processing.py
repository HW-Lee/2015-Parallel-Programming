import sys
import re
import getopt

def process_opts(argv, spec_opts, usage_str="", default=None):

    short_opts = filter( lambda x: len(x[0]) == 1 ,spec_opts )
    long_opts = filter( lambda x: len(x[0]) > 1, spec_opts )

    opts, args = getopt.getopt( argv, "".join( map( lambda x: "".join(x), short_opts ) ), map( lambda x: "".join(x), long_opts ) )
    opts = dict( zip( map( lambda x: re.sub("^-+", "", x[0]), opts ), map( lambda x: x[1], opts ) ) )

    return (opts, args)


def test():

    OPTS = [
        ("o", ":"),
        ("i", ""),
        ("orig", "="),
        ("dest", "=")
    ]

    print( process_opts( ["-i", "-o", "output", "--orig", "original"], OPTS ) )

if __name__ == "__main__": test()