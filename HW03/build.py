import os
import sys

def main():
	versions = [ "MPI", "OpenMP", "Hybrid" ]
	methods = [ "static", "dynamic" ]

	pairs = map( lambda x: (x, methods), versions )
	filelist = map( lambda x: map( lambda y: "MS_" + x[0] + "_" + y, x[1] ), pairs )
	filelist = reduce( lambda x, y: x + y, filelist )

	filelist = filelist + ["MS_MPI_pthread_dynamic"]

	map( build_code, filelist )

def build_code(path):
	full_path = os.path.dirname( os.path.abspath( __file__) ) + "/" + path

	if os.path.exists( full_path + ".c" ):
		
		if path.startswith("MS_OpenMP"): compiler = "g++ -O3 -lX11 "
		else: compiler = "mpic++ -lX11 -pthread "
		if not path.startswith("MS_MPI"): compiler += "-fopenmp "

		os.system( compiler + path + ".c -o " + path )

if __name__ == "__main__": main()
