import os
import sys

def main():
    NVCC = "nvcc -arch=sm_20 -O3"
    compile_pair = [
        ("HW4_103061527_cuda", NVCC),
        ("HW4_103061527_openmp", NVCC + " -Xcompiler=\"-fopenmp\""),
        ("HW4_103061527_mpi", NVCC + " -I /usr/include/mpich-x86_64 -L /usr/lib64/mpich/lib -lmpich")
    ]

    map(build_code, compile_pair)

def build_code(pair):
    path, build_prefix = pair
    full_path = os.path.dirname( os.path.abspath( __file__) ) + "/" + path

    if os.path.exists(full_path + ".cu"):
        os.system(build_prefix + " " + path + ".cu -o " + path)

if __name__ == "__main__": main()
