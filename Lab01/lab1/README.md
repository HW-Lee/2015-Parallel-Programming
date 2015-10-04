*Parallel Programming 2015*

1. QUICK REFERENCES
# submit a job:
> qsub pbs.sh

# delete a job:
> qdel jobid

# monitor jobs:
> qstat -a jobid


2. About job queue constraints for each user:
- debug --- for quick debugging purpose
    *  Max nodes = 2
    *  Max total processes = 8
    *  Max walltime = 5 minute
    *  Max jobs queuable at any time = 2
    *  Max jobs runnable at any time = 1

- batch --- for benchmarking purpose
    *  Max nodes = 4
    *  Max total processes = 96
    *  Max walltime = 30 minutes
    *  Max jobs queuable at any time = 8
    *  Max jobs runnable at any time = 2

3. About job scheduling:
- When there are no much free resources available, the scheduler will
    *  favor short running jobs (based on walltime)
    *  favor less resource demanding jobs (based on nodes, ppn)
    *  favor jobs which are queued for a long time
    *  favor debug jobs over batch jobs
- But these rules not necessarily absolute. For example:
    *  A short job may be executed after a long running job since the latter requires only 1 process.
    *  A debug job may be executed after a batch job since the latter has been queued for 2 hours.
	*  ...

Be sure to request *reasonable* amount of resources according to your own requirements.
if you have any question, please ask on [iLMS](http://lms.nthu.edu.tw) or email [TA](jacoblee@lsalab.cs.nthu.edu.tw)
Thanks!

