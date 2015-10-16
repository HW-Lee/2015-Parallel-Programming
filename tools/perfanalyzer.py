import sys
import os
import glob
import json
import re
import getopt
import numpy as np
import matplotlib.pyplot as pyplot


class Performance( object ):
	def __init__( self, jhandle_arr, res_info ):
		self.processors = sorted( jhandle_arr, key=lambda x: x["id"] )
		map( lambda x: x.pop( "id" ), self.processors )
		self.exec_time = float( self._cal_exec_time() )
		self.res_info = res_info

	def _cal_exec_time( self ):
		return max( map( lambda x: sum( np.array( x.values() ) ), self.processors ) )

	def get_detail_exec_info( self ):
		values = np.array( map( lambda x: map( float, x.values() ), self.processors ) )
		values = map( lambda x: values[:, x], range( len( self.processors[0].keys() ) ) )
		mean_std_pairs = zip( map( np.mean, values ), map( np.std, values ) )
		return dict( zip( self.processors[0].keys(), mean_std_pairs ) )

def glob_perf( folder ):
	if not folder.endswith( "/" ): folder += "/"
	file_list = glob.glob( folder + "*.json" )

	res_info_list = map( get_res_info, file_list )
	file_list = zip( file_list, res_info_list )

	file_list_single = filter( lambda x: x[1][0] == 1, file_list )
	file_list_multiple = filter( lambda x: x[1][0] > 1, file_list )

	file_list_single = sorted( file_list_single, key=lambda x: x[1][0]*x[1][1] )
	file_list_multiple = sorted( file_list_multiple, key=lambda x: x[1][0]*x[1][1] )

	perf_list_single = map( Performance, map( load_json, file_list_single ), map( lambda x: x[1], file_list_single ) )
	perf_list_multiple = map( Performance, map( load_json, file_list_multiple ), map( lambda x: x[1], file_list_multiple ) )

	return [ perf_list_single, perf_list_multiple ]


def get_res_info( path ):
	pattern = "nodes=[0-9]+:ppn=[0-9]+"
	matched = re.search( pattern, path ).group(0)
	return map( int, re.sub( "[(nodes=)(ppn=)]", "", matched ).split( ":" ) )

def load_json( dummy_file ):
	return json.load( file( dummy_file[0] ) )

def get_exec_time( perf_list ):
	return map( lambda dummy: map( lambda perf: perf.exec_time, dummy ), perf_list )

def get_exec_info( perf_list ):
	return map( _dummy_get_exec_info, perf_list )

def _dummy_get_exec_info( perf_list ):
	ex_hashes = map( lambda x: x.get_detail_exec_info(), perf_list )
	keys = [ "comm", "comp", "sync", "i/o" ]
	info_list = map( lambda key: np.array( map( lambda x: x[key], ex_hashes ) ), keys )
	
	return ( dict( zip( keys, info_list ) ), map( lambda x: x.res_info, perf_list ) )


if __name__ == "__main__": main( sys.argv )
