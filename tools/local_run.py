import os
import sys
import opt_processing as op
import re

def main(argv):
    import getopt

    OPTS = [
        ("h", ""),
        ("t", ""),
        ("n", ":"),
        ("args", "=")
    ]

    USAGE = "[Usage] [ -t time ] [ -n specified node # ] [ --args 'argv1 argv2 argv3 ...' ] [ file1.c file2.c file3.c ... ]"

    try:
        opts, args = op.process_opts( argv[1:], OPTS, USAGE )
    except getopt.GetoptError as err: print( err )

    if "h" in opts.keys():
        print( USAGE )
        exit(0)

    if "t" in opts.keys(): run_cmd = "time "
    else: run_cmd = ""

    compile_cmd = "mpicc"
    run_cmd += "mpiexec"
    remove_cmd = "rm"
    filenames = args
    outfilenames = map( lambda x: re.sub( "\.c", ".o", x ), filenames )

    # Compiling files
    cmds = map( _generate_cmd, [compile_cmd] * len(filenames), outfilenames, filenames )
    map( _print, cmds )
    map( os.system, cmds )

    # Executing executables
    node_opt = opts.get( "n", "" )
    if len(node_opt) > 0: node_opt = " -npernode " + node_opt
    cmds = map( lambda x: run_cmd + node_opt + " " + x + " " + opts.get( "args", "" ) + " > " + re.sub( "\.o", ".log", x ), outfilenames )
    map( _print, cmds )
    map( os.system, cmds )

    # Deleting executables
    cmds = map( lambda x: remove_cmd + " " + x, outfilenames )
    map( _print, cmds )
    map( os.system, cmds )


def _generate_cmd( suffix, outfile, infile ):
    return suffix + " -o " + outfile + " " + infile

def _print(n):
    print(n)
    return n

if __name__ == "__main__": main(sys.argv)