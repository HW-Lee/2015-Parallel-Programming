import os
import sys
import opt_processing
import gen_job_script as job
import json
import shutil
import numpy as np

def main(argv):
    import getopt

    OPTS = [
        ("h", ""),
        ("dir", "="),
        ("config", "=")
    ]
    USAGE = "\n".join( map( lambda x: "".join(x), OPTS ) )

    try:
        opts, args = opt_processing.process_opts( argv[1:], OPTS, USAGE )
    except getopt.GetoptError as err: print( err )

    if "h" in opts.keys() or "config" not in opts.keys():
        print( USAGE )
        exit(0)

    out_dir = opts.get( "dir", "run_space" )
    if out_dir.endswith( "/" ): out_dir = out_dir[0:-1]
    config_file = file( opts[ "config" ] )

    if os.path.exists( out_dir ) and os.path.isdir( out_dir ): shutil.rmtree( out_dir )
    os.makedirs( out_dir )

    config = json.load( config_file )
    queue_mode = config[ "queue_mode" ]
    job_name = config[ "job_name" ]
    rerunnable = config[ "rerunnable" ]
    exec_cmd = config[ "exec_cmd" ]
    outpath_prefix = config[ "outpath_prefix" ]
    x11_forwarding = config.get( "x11_forwarding", False )

    res_configs = config[ "res_configs" ]
    n = len( res_configs )
    outpaths = map( lambda x: "/".join( [ out_dir, outpath_prefix + str(x) + ".sh" ] ), np.arange( 0, n ) + 1 )
    map( _dummy_gen_job_script, [queue_mode] * n, [job_name] * n, [rerunnable] * n, [exec_cmd] * n, outpaths, x11_forwarding, res_configs )

    exit(0)

def _dummy_gen_job_script( queue_mode, job_name, rerunnable, exec_cmd, outpath, x11_forwarding, res_config ):
    nodes_num = str( res_config[ "nodes_num" ] )
    ppn = str( res_config[ "ppn" ] )
    wt = str( res_config[ "wt" ] )

    job.gen_job_script( queue_mode, job_name, rerunnable, nodes_num, ppn, wt, exec_cmd, outpath)


if __name__ == "__main__": main(sys.argv)