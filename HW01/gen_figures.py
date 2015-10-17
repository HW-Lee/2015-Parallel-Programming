import sys
import os
import numpy as np
import matplotlib.pyplot as pyplot

sys.path.append( os.path.join( os.path.dirname(__file__), '..', 'tools' ) )

import perfanalyzer as pf

def main():
	perf_list_basic = pf.glob_perf( "run_report/basic" )
	perf_list_advanced_bubble = pf.glob_perf( "run_report/advanced/bubble" )
	perf_list_advanced_merge = pf.glob_perf( "run_report/advanced/merge" )

	exec_time_info_basic = pf.get_exec_info( perf_list_basic )
	exec_time_info_advanced_bubble = pf.get_exec_info( perf_list_advanced_bubble )
	exec_time_info_advanced_merge = pf.get_exec_info( perf_list_advanced_merge )

	gen_exec_time_figures( exec_time_info_basic[0], "run_report/basic/single" )
	gen_exec_time_figures( exec_time_info_basic[1], "run_report/basic/multiple" )

	gen_exec_time_figures( exec_time_info_advanced_bubble[0], "run_report/advanced/bubble/single" )
	gen_exec_time_figures( exec_time_info_advanced_bubble[1], "run_report/advanced/bubble/multiple" )

	gen_exec_time_figures( exec_time_info_advanced_merge[0], "run_report/advanced/merge/single" )
	gen_exec_time_figures( exec_time_info_advanced_merge[1], "run_report/advanced/merge/multiple" )

	exec_time_single_list = map( lambda x: pf.get_exec_time(x)[0], [ perf_list_basic, perf_list_advanced_merge, perf_list_advanced_bubble ] )
	gen_speedup_figures( exec_time_single_list )

	exit(0)

def gen_exec_time_figures( hash_n_res, name ):
	exec_time_info_hash, res = hash_n_res

	arrs = map( lambda key: exec_time_info_hash[key][:, 0], [ "comp", "comm", "sync", "i/o" ] )
	printed = zip( arrs, ["g", "b", "y", "r"], ["Computation", "Communication", "Synchronization", "I/O"] )

	X = np.arange( 1, len( exec_time_info_hash.values()[0] )+1 )
	acc_bottom = np.zeros( np.shape( exec_time_info_hash.values()[0][:, 0] ) )

	for each in printed:
		arr, c, l = each
		pyplot.bar( X, arr, color=c, align="center", label=l, bottom=acc_bottom )
		acc_bottom += arr

	pyplot.ylim( 0, max( max( acc_bottom ), max( acc_bottom[-1:] )*1.3 ) )

	pyplot.xticks( X, map( lambda x: x[0]*x[1], res ) )
	pyplot.xlabel( "# of processors" )
	pyplot.ylabel( "ms" )
	pyplot.legend()

	pyplot.gcf().set_size_inches( 12, 9 )

	pyplot.savefig( name + "-exec.png", transparent=True, pad_inches=0 )
	pyplot.gcf().clear()

def gen_speedup_figures( time_n_res_list ):
	props = [ 1., 1., 2. ]
	labels = [ "basic", "advanced-merge", "advanced-bubble" ]
	linestyles = [ "gs-", "bs-", "rs-" ]
	task_max = max( map( _dummy_add_speedup_plot, time_n_res_list, props, labels, linestyles ) )

	tasks_list = np.arange( 1, task_max+1 )
	pyplot.plot( tasks_list, tasks_list, "k--", label="Ideal" )

	pyplot.ylim( 0, task_max*1.3 )

	pyplot.xlabel( "# of processors" )
	pyplot.ylabel( "speedup" )
	pyplot.legend()

	pyplot.gcf().set_size_inches( 12, 9 )
	pyplot.savefig( "run_report/speedup.png", transparent=True, pad_inches=0 )
	pyplot.gcf().clear()

def _dummy_add_speedup_plot( time_n_res, prop , label, linestyle ):
	time, res = time_n_res
	speedup_list = map( lambda x: np.power( time[0]/x, 1./prop ), time )
	tasks_list = map( lambda x: x[0]*x[1], res )
	pyplot.plot( tasks_list, speedup_list, linestyle, label=label )

	return max( tasks_list )


if __name__ == "__main__": main()