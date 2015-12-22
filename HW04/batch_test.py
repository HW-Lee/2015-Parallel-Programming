import sys
import os
import time

def main(executable):
    params_list = []
    params_list = params_list + map(lambda x: (executable, "testcase/in1", x+1, "testcase/ans1"), range(64))
    params_list = params_list + map(lambda x: (executable, "testcase/in2", x+1, "testcase/ans2"), range(64))
    params_list = params_list + map(lambda x: (executable, "testcase/in3", x+1, "testcase/ans3"), range(64))
    params_list = params_list + map(lambda x: (executable, "testcase/in4", 2**(x+1), "testcase/ans4"), range(6))
    params_list = params_list + map(lambda x: (executable, "testcase/in5", 2**(x+4), "testcase/ans5"), range(3))

    map(run_cmd, params_list)
    print("")

def run_cmd(params):
    executable, infile, blocksize, ans = params
    cmd = executable + " " + infile + " outfile " + str(blocksize) + " > /dev/null"

    if executable.split("_")[-1] == "mpi": cmd = "/usr/lib64/mpich/bin/mpirun -np 2 -hostfile hostfile " + cmd
    
    sys.stdout.write("\rRun cmd: " + executable + " " + infile + " outfile " + str(blocksize) + " ")
    sys.stdout.flush()

    os.system(cmd)
    outfile_path = os.path.dirname(os.path.abspath(__file__)) + "/outfile"

    while not os.path.exists(outfile_path): time.sleep(0.5)

    resp = os.system("diff -b outfile " + ans + " > /dev/null")
    os.system("rm -f " + outfile_path)

    if resp > 0:
        sys.stdout.write("Oops!\n")
        sys.stdout.flush()

if __name__ == "__main__": main(sys.argv[1])
