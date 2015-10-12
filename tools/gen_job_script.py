import os
import sys
import opt_processing

def main(argv):
    import getopt

    OPTS = [
        ("h", ""),
        ("o", ":"),
        ("d", ""),
        ("b", ""),
        ("N", ":"),
        ("r", ""),
        ("nodes", "="),
        ("ppn", "="),
        ("wt", "=")
    ]
    USAGE = "\n".join( map( lambda x: "".join(x), OPTS ) )

    try:
        opts, args = opt_processing.process_opts( argv[1:], OPTS, USAGE )
    except getopt.GetoptError as err: print( err )

    if "h" in opts.keys():
        print( USAGE )
        exit(0)

    queue_mode = "debug"
    rerunnable = "n"

    if "b" in opts.keys(): queue_mode = "batch"
    if "r" in opts.keys(): rerunnable = "y"
    job_name = opts.get( "N", "MY_JOB" )
    nodes_num = opts.get( "nodes", "1" )
    ppn_num = opts.get( "ppn", "4" )
    tasks = "-l nodes=" + nodes_num + ":ppn=" + ppn_num
    wt = opts.get( "wt", "00:01:00" )

    outpath = opts.get( "o", "job" )
    if not outpath.endswith( ".sh" ): outpath += ".sh"
    gen_job_script( queue_mode, job_name, rerunnable, nodes_num, ppn_num, wt, " ".join( args[0:] ), outpath )



def gen_job_script(queue_mode="debug", job_name="MY_JOB", rerunnable="n", nodes_num="1", ppn_num="4", wt="00:01:00", exec_cmd=None, outpath="job.sh"):

    OPT_PREFIX = "#PBS"

    info = [
        "-q " + queue_mode,
        "-N " + job_name,
        "-r " + rerunnable,
        "-l " + "nodes=" + nodes_num + ":ppn=" + ppn_num,
        "-l " + "walltime=" + wt,
    ]

    info = map( lambda x: OPT_PREFIX + " " + x, info )
    info.append( "cd $PBS_O_WORKDIR" )
    info.append( "time mpiexec " + exec_cmd )

    if __name__ == "__main__": print( "\n".join(info) )

    with open( outpath, "w" ) as f:
        f.write( "\n".join(info) )

if __name__ == "__main__": main(sys.argv)