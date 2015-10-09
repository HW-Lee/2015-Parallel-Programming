#PBS -q debug
#PBS -N HW1_job
#PBS -r n
#PBS -l nodes=1:ppn=5
#PBS -l walltime=00:01:00
cd $PBS_O_WORKDIR
time mpiexec ./HW1_103061527 97 ../../shared/hw1/testcase/testcase7 ./output_test
