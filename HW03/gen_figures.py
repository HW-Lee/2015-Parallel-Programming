import sys
import os
import json
import numpy as np
import matplotlib.pyplot as pyplot

def main():
    with open("exp.json") as data_file:
        data = json.load(data_file)

    MPI_command = data["MPI_command"]
    OpenMP_command = data["OpenMP_command"]
    Hybrid_command = data["Hybrid_command"]
    MPI_static_res_info = data["MPI_static"]
    MPI_dynamic_res_info = data["MPI_dynamic"]
    OpenMP_static_res_info = data["OpenMP_static"]
    OpenMP_dynamic_res_info = data["OpenMP_dynamic"]
    Hybrid_static_res_info = data["Hybrid_static"]
    Hybrid_dynamic_res_info = data["Hybrid_dynamic"]

    elapsed_static = data["elapsed-detail_static"]
    elapsed_dynamic = data["elapsed-detail_dynamic"]

    var_resources = data["var_resources"]
    var_problemsize = data["var_problemsize"]


    print( "Generating threads_load figures..." )
    gen_threads_load_figure( MPI_static_res_info, MPI_dynamic_res_info, MPI_command, "MPI" )
    gen_threads_load_figure( OpenMP_static_res_info, OpenMP_dynamic_res_info, OpenMP_command, "OpenMP" )
    gen_threads_load_figure( Hybrid_static_res_info, Hybrid_dynamic_res_info, Hybrid_command, "Hybrid" )

    print( "Generating load_dist figures..." )
    gen_columns_load_figure( elapsed_static, "static" )
    gen_columns_load_figure( elapsed_dynamic, "dynamic" )

    print( "Generating speedup figure..." )
    gen_speedup_figure( var_resources )

    print( "Generating Scalability figure..." )
    gen_exec_figure( var_resources )
    gen_problemsize_figure( var_problemsize )

    print( "Generating execution time figure of multi-nodes..." )
    gen_multinodes_exec_figure( json.load(open("Hybrid_static_combinations.json")), json.load(open("Hybrid_dynamic_combinations.json")) )

def gen_threads_load_figure( static, dynamic, command, name ):
    width = 0.1
    x = np.array( map( lambda x: x["id"], static ) ) + 1
    comp = np.array( map( lambda x: x.get("comp_millis", 0), static ) ) # g
    comm = np.array( map( lambda x: x.get("comm_millis", 0), static ) ) # b
    sync = np.array( map( lambda x: x.get("sync_millis", 0), static ) ) # y

    pyplot.bar( x - 2*width, comp, width, color="g", label="computation", align="center" )
    pyplot.bar( x - 2*width, comm, width, color="b", label="communication", bottom=comp, align="center" )
    rects = pyplot.bar( x - 2*width, sync, width, color="y", label="synchronization", bottom=comp+comm, align="center" )
    offset = comp + comm

    for i in range(len(rects)):
        rect = rects[i]
        h = rect.get_height() + offset[i]
        pyplot.text( rect.get_x() + rect.get_width()/2., h, "static", ha="center", va="bottom" )

    pyplot.ylim(0, max(comp)*1.3)

    x = np.array( map( lambda x: x["id"], dynamic ) ) + 1
    comp = np.array( map( lambda x: x.get("comp_millis", 0), dynamic ) )
    comm = np.array( map( lambda x: x.get("comm_millis", 0), dynamic ) )
    sync = np.array( map( lambda x: x.get("sync_millis", 0), dynamic ) )

    pyplot.bar( x + 2*width, comp, width, color="g", align="center" )
    pyplot.bar( x + 2*width, comm, width, color="b", bottom=comp, align="center" )
    rects = pyplot.bar( x + 2*width, sync, width, color="y", bottom=comp+comm, align="center" )
    offset = comp + comm

    for i in range(len(rects)):
        rect = rects[i]
        h = rect.get_height() + offset[i]
        pyplot.text( rect.get_x() + rect.get_width()/2., h, "dyn", ha="center", va="bottom" )

    pyplot.xticks( x, x )

    pyplot.legend()
    pyplot.title( "command: " + command )
    pyplot.xlabel( "workers ID" )
    pyplot.ylabel( "time (ms)" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/threads_load_" + name + ".png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_columns_load_figure( data, name ):
    data = np.array( map( lambda x: [x[0]] + x[1], zip( range(len(data)), data ) ) )
    idxes = data[:, [0]]
    contributors = data[:, [1]]
    num_contributors = max( contributors ) + 1

    for i in range(num_contributors):
        loads = filter( lambda x: x[1] == i, data )
        loads = map( lambda x: x[ [0, 2] ], loads )
        loads = np.array( loads )

        markerline, stemline, baseline = pyplot.stem( loads[:, 0], loads[:, 1], align="center", label="worker"+str(i)+": "+str(len(loads)) )
        pyplot.setp( stemline, linewidth=1, color=str(0.8*float(i)/float(num_contributors)) )
        pyplot.setp( markerline, 'markerfacecolor', str(0.8*float(i)/float(num_contributors)) )

    pyplot.yscale( "log" )
    pyplot.legend()
    pyplot.title( "Load distribution w.r.t. column indices of " + name + " version" )
    pyplot.xlabel( "column number" )
    pyplot.ylabel( "computation time (ms)" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/load_dist_" + name + ".png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_speedup_figure( data ):
    dim_array = [ "MPI_static", "MPI_dynamic", "OpenMP_static", "OpenMP_dynamic" ]
    colors = [ "r", "g", "b", "m" ]
    h = map( lambda x: (x, np.array(data[x+"_elapsed"])), dim_array )
    h = dict( h )

    for i in range(len(dim_array)): 
        key = dim_array[i]
        h[key] = np.power( h[key] / h[key][0], -1 )

        pyplot.plot( np.arange( len(h[key]) ) + 1, h[key], colors[i]+"o-", label=key )

    pyplot.plot( range( len(h[key]) + 1 ), range( len(h[key]) + 1 ), "k--", label="Ideal" )

    pyplot.legend( loc="upper left" )
    pyplot.title( "Speedup Results" )
    pyplot.xlabel( "# of workers" )
    pyplot.ylabel( "speedup factor" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/speedup.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_exec_figure( data ):
    dim_array = [ "MPI_static", "MPI_dynamic", "OpenMP_static", "OpenMP_dynamic" ]
    colors = [ "r", "g", "b", "m" ]
    h = map( lambda x: (x, np.array(data[x+"_elapsed"])), dim_array )
    h = dict( h )

    for i in range(len(dim_array)): 
        key = dim_array[i]
        pyplot.plot( np.arange( len(h[key]) ) + 1, h[key], colors[i]+"o-", label=key )

    pyplot.legend()
    pyplot.title( "Execution time" )
    pyplot.xlabel( "# of workers" )
    pyplot.ylabel( "time (sec)" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/exec.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_problemsize_figure( data ):
    dim_array = [ "MPI_static", "MPI_dynamic", "OpenMP_static", "OpenMP_dynamic" ]
    colors = [ "r", "g", "b", "m" ]
    h = map( lambda x: (x, np.array(data[x+"_elapsed"])), dim_array )
    h = dict( h )

    max_y = 0;
    for i in range(len(dim_array)): 
        key = dim_array[i]
        h[key] = np.power( h[key]*1000, 0.5 )

        pyplot.plot( 100 * (np.arange( len(h[key]) ) + 1), h[key], colors[i]+"o-", label=key )
        if max_y < max(h[key]): max_y = max(h[key])


    yticks = np.arange( np.ceil( max_y / 20. ) + 1 ) * 20
    pyplot.yticks( yticks, np.power( yticks, 2 ) )

    xticks = np.arange(6) * 200
    pyplot.xticks( xticks, np.power( xticks, 2 ) )

    pyplot.legend( loc="upper left" )
    pyplot.title( "Weak Scalability" )
    pyplot.xlabel( "problem size (# points)" )
    pyplot.ylabel( "time (ms)" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/problemsize.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_multinodes_exec_figure( static, dynamic ):
    for x in range(2):
        if x == 1: 
            data = static
            postfix = "static"
            marker = "o"
        else:
            data = dynamic
            postfix = "dynamic"
            marker = "^"

        list_wrt_num_nodes = map( lambda x: filter( lambda y: y["nnodes"] == x, data ), np.arange(4)+1 )
        exec_time_wrt_num_nodes = map( lambda x: map( lambda y: y["exec_e"], x ), list_wrt_num_nodes )
        color_list = [ "r", "g", "b", "m" ]

        for i in range(len(exec_time_wrt_num_nodes)):
            if i < 1: continue
            linestyle = color_list[i] + marker + "-" + "-" * x
            label = "nnodes = " + str(i+1) + " [" + postfix + "]"
            pyplot.plot( (i+1) * (np.arange( len(exec_time_wrt_num_nodes[i]) )+1), exec_time_wrt_num_nodes[i], linestyle, label=label )

    # pyplot.ylim(0, map( lambda x: x["exec_e"], data ) * 1.3)
    pyplot.legend()
    pyplot.title( "Execution time w.r.t different resource configuration" )
    pyplot.xlabel( "# of workers" )
    pyplot.ylabel( "time (sec)" )
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/res_dist_exec.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

if __name__ == "__main__": main()
