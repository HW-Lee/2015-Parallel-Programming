#PBS -q debug
#PBS -N HW1_job
#PBS -r n
#PBS -l nodes=1:ppn=4
#PBS -l walltime=00:01:00
cd $PBS_O_WORKDIR
time mpiexec ./HW1_103061527 30 ./input_test ./output_test
