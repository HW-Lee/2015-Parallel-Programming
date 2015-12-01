#PBS -q batch
#PBS -N Hybrid
#PBS -r n
#PBS -l nodes=VAR(NNODES):ppn=12
#PBS -l walltime=00:05:00

cd $PBS_O_WORKDIR
OMP_NUM_THREADS=$PBS_NUM_PPN
export OMP_NUM_THREADS=12
export MV2_ENABLE_AFFINITY=0
NUM_MPI_PROCESS_PER_NODE=1

time mpiexec -ppn $NUM_MPI_PROCESS_PER_NODE ./MS_Hybrid_dynamic VAR(NTHREADS) -2 2 -2 2 800 800 disable