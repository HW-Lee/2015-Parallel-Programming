import os
import sys
import time
import getopt
import glob
import re

def main( argv ):

	try:
		opts, args = getopt.getopt( argv[1:], "h", [ "dir=" ] )
		opts = dict( opts )
	except getopt.GetoptError as err:
		opts = {}
		opts[ "-h" ] = ""

	if "-h" in opts.keys() or "--dir" not in opts.keys():
		print_usage()
		exit(0)

	if opts[ "--dir" ].endswith( "/" ): opts[ "--dir" ] = opts[ "--dir" ][0:-1]

	sh_list = glob.glob( opts[ "--dir" ] + "/*.sh" )
	resources_list = map( _get_resources, sh_list )
	job_names_list = map( _get_job_name, sh_list )

	pkg_list = zip( sh_list, map( lambda x: [x[0]] + x[1],zip( job_names_list, resources_list ) ) )

	single_node_list = filter( lambda x: x[1][1] is 1, pkg_list )
	multi_node_list = filter( lambda x: x[1][1] > 1, pkg_list )

	map( _print, single_node_list )
	print( "------------------------" )
	map ( _print, multi_node_list )
	print( "------------------------" )

	map( _dummy_run_script, single_node_list )
	map( _dummy_run_script, multi_node_list )
	print( "" )

	exit(0)


def print_usage():
	print( "Usage: -h for help, --dir DIR_PATH" )

def _get_resources( path ):
	job_str = ""
	with open( path, "r" ) as f: job_str = f.read()
	nodes_num, ppn = re.sub( "[(nodes=)(ppn=)]", "", re.search( "nodes=[0-9]+?:ppn=[0-9]+?", job_str ).group( 0 ) ).split( ":" )

	return [ int( nodes_num ), int( ppn ) ]

def _get_job_name( path ):
	job_str = ""
	with open( path, "r" ) as f: job_str = f.read()
	name = re.sub( "-N ", "", re.search( "-N .*?\w+", job_str ).group( 0 ) )

	return name

def _dummy_run_script( pkg ):
	path = pkg[0]
	job_name = pkg[1][0]
	nodes_num = pkg[1][1]
	ppn = pkg[1][2]

	print( "Running " + path + " ..." )
	os.system( "qsub " + path + " > /dev/null" )

	while len( glob.glob( job_name + ".*" ) ) < 2: time.sleep( .2 )

	_process_file( glob.glob( job_name + ".o*" )[0], job_name, nodes_num, ppn )
	os.system( "rm " + job_name + ".*" )

def _process_file( path, job_name, nodes_num, ppn ):
	file_name = str( job_name ) + "_nodes=" + str( nodes_num ) + ":ppn=" + str( ppn ) + ".json"
	with open( path, "r" ) as f: content = f.read()
	with open( file_name, "w" ) as f: f.write( "[" + "},".join( content.split( "}" )[0:-1] ) + "}]\n" )

	print( "generate report: " + file_name )

def _print( arg ):
	print( arg )
	return None

if __name__ == "__main__": main( sys.argv )