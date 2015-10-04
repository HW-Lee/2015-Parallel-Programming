
# NOTICE: Please do not remove the '#' before 'PBS'

# Select which queue (debug, batch) to run on
#PBS -q debug

# Name of your job
#PBS -N MY_JOB

# Declaring job as not re-runnable
#PBS -r n

# Resource allocation (how many nodes? how many processes per node?)
#PBS -l nodes=1:ppn=8

# Max execution time of your job (hh:mm:ss)
# Your job may got killed if you exceed this limit
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
time mpiexec ./sum 84000000 /home/pp2015/shared/IN_84000000 # edit this line to fit your needs!
