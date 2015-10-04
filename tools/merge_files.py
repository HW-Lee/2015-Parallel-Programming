import os
import sys
import opt_processing as op

def main(argv):
    import getopt

    OPTS = [
        ("h", ""),
        ("d", ""),
        ("o", ":")
    ]

    USAGE = "[Usage] [-d delete files after merging] [-o merged file] [outfile file1 file2 file3 ...  input files]"

    try:
        opts, args = op.process_opts( argv[1:], OPTS, USAGE )
    except getopt.GetoptError as err: print( err )


    if "h" in opts.keys():
        print( USAGE )
        exit(0)

    filenames = args
    outfile = opts.get( "o", "outfile_" + "".join( filenames ) )

    filecontents = zip( filenames, map( lambda filename: file( filename ).read(), filenames ) )

    if "d" in opts.keys():
        cmd = "rm "
        map( lambda filename: os.system( cmd + filename ), filenames )

    with open( outfile, "w" ) as f:
        f.write( "--------------------\n" )
        f.write( "\n".join( map( lambda x: ":\n\n".join( x ) + "\n--------------------", filecontents ) ) )



if __name__ == "__main__": main(sys.argv)