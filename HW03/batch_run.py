import sys
import os
import time
import glob
import json

def main():
    with open( "template.sh" ) as f: template = f.read()

    scripts_list = []

    for nnodes in range(4):
        if nnodes < 1: continue
        job_scripts = template.replace( "VAR(NNODES)", str(nnodes+1) )
        for nthreads in range(12):
            if (nnodes+1) * (nthreads+1) > 25: continue
            scripts_list.append( [ nnodes+1, nthreads+1, job_scripts.replace( "VAR(NTHREADS)", str(nthreads+1) ) ] )

    json_h = map( dummy_run, scripts_list )
    
    print( "\nGenerating 'report.json'" )
    with open( "report.json", "w" ) as f: f.write( json.dumps( json_h, sort_keys=True, indent=4 ) + "\n" )


def dummy_run( data ):
    scripts = data[2]
    with open( "dummy.sh", "w" ) as f: f.write( scripts )
    os.system( "qsub dummy.sh > /dev/null" )

    sys.stdout.write( "\rProcess nnodes=" + str(data[0]) + ", nthreads=" + str(data[1]) + "..." )
    sys.stdout.flush()

    while len( glob.glob( "Hybrid.*" ) ) < 2: time.sleep( .2 )
    os.system( "rm dummy.sh" )

    h = {
        "nnodes": data[0],
        "nthreads": data[1],
        "exec_o": open( glob.glob( "Hybrid.o*" )[0] ).read().split( "\n" ),
        "exec_e": open( glob.glob( "Hybrid.e*" )[0] ).read().split( "\n" )
    }

    os.system( "rm Hybrid.*" )

    return h

if __name__ == "__main__": main()