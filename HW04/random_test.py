import os
import sys

def main(argv):
    executable = argv[2]
    testsize = int(argv[0])
    iterations = int(argv[1])

    cmd = executable
    if executable.split("_")[-1] == "mpi": cmd = "/usr/lib64/mpich/bin/mpirun -np 2 " + cmd

    for x in range(1, iterations+1):
        sys.stdout.write("\r[" + str(x) + "]Generating testcase...")
        sys.stdout.flush()
        os.system("python gen_testcase.py " + str(testsize))
        sys.stdout.write("\r[" + str(x) + "]Generating answer...")
        sys.stdout.flush()
        os.system("./a.out test" + str(testsize) + " ans 32")
        sys.stdout.write("\r[" + str(x) + "]Evaluating accuracy...")
        sys.stdout.flush()
        os.system(cmd + " test" + str(testsize) + " outfile 32 > /dev/null")
        resp = os.system("diff -b outfile ans > /dev/null")
        if resp > 0:
            sys.stdout.write("Oops!\n")
            sys.stdout.flush()
            os.system("mv test" + str(testsize) + " test_err" + str(x))
            os.system("mv ans ans_err" + str(x))
        else: 
            os.system("rm -f outfile")
            os.system("rm -f ans")
    print("")

if __name__ == "__main__": main(sys.argv[1:])
