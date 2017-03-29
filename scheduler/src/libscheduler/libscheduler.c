/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.
  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
  int pid;
  int arrival_time;
  int priority;
  int used_time;
  int total_time_needed;
  int last_start_time;
  int job_response_time;
} job_t;

int remainingTime(const job_t *job)
{
    return job->total_time_needed - job->used_time;
}

typedef struct _scheduler_t
{
  scheme_t scheduler_scheme;
  priqueue_t job_queue;

  int core_count;
  job_t** current_jobs_on_cores;

  int total_wait_time;
  int total_response_time;
  int total_turn_around_time;
  int total_jobs_count;
} scheduler_t;

scheduler_t *scheduler_ptr;


//These Sort the Queues
int FCFScompare(const void *a, const void *b)
{
  return -1;
}

int SJFcompare(const void *a, const void *b)
{
  job_t const *left = (job_t*)a;
  job_t const *right = (job_t*)b;

  if( remainingTime(left) == remainingTime(right) )
  {
       return ( left->arrival_time - right->arrival_time ) ;
  }
  else
  {
       return ( remainingTime(left) - remainingTime(right) ) ;
  }
}

int PRIcompare(const void *a, const void *b)
{
  job_t const *left = (job_t*)a;
  job_t const *right = (job_t*)b;

  if( left->priority == right->priority)
  {
    return (right->arrival_time - left->arrival_time) * -1;
  }
  else
  {
    return (right->priority - left->priority) * -1;
  }

}

/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
    scheduler_ptr = (scheduler_t *) calloc( 1, sizeof(scheduler_t) );
	scheduler_ptr->core_count = cores;
	scheduler_ptr->current_jobs_on_cores = (job_t **) calloc( cores , sizeof(job_t*) );
    scheduler_ptr->scheduler_scheme = scheme;

	switch(scheme)
	{
		case FCFS:
		case RR:
			priqueue_init(&scheduler_ptr->job_queue, FCFScompare);
			break;
		case SJF:
		case PSJF:
			priqueue_init(&scheduler_ptr->job_queue, SJFcompare);
			break;
		case PRI:
		case PPRI:
			priqueue_init(&scheduler_ptr->job_queue, PRIcompare);
			break;
	}
}

int idleCore()
{
	for(int i = 0; i< scheduler_ptr->core_count; i++)
	{
		if( scheduler_ptr->current_jobs_on_cores[i] == NULL )
		{
			return i;
		}
	}
	return -1;
}

int findLongestRemainingJob()
{
    int core = -1;
    int longest_length = 0;
    int arrival_time = 0;
    for(int i = 0; i< scheduler_ptr->core_count; i++)
    {
        if( scheduler_ptr->current_jobs_on_cores[i] != NULL && remainingTime( scheduler_ptr->current_jobs_on_cores[i] ) > longest_length )
        {
            longest_length = remainingTime( scheduler_ptr->current_jobs_on_cores[i] );
            arrival_time =  scheduler_ptr->current_jobs_on_cores[i]->arrival_time;
            core = i;
        }
        else if ( remainingTime( scheduler_ptr->current_jobs_on_cores[i] ) == longest_length )
        {
            if( scheduler_ptr->current_jobs_on_cores[i]->arrival_time >  arrival_time )
            {
                arrival_time = scheduler_ptr->current_jobs_on_cores[i]->arrival_time;
                core = i;
            }
        }
    }
    return core;
}

int findWorstPriorityJob()
{
    int core = -1;
    int worst_priority = 0;
    int arrival_time = 0;
    for(int i = 0; i< scheduler_ptr->core_count; i++)
    {
        if( scheduler_ptr->current_jobs_on_cores[i] != NULL)
        {
            if( scheduler_ptr->current_jobs_on_cores[i]->priority > worst_priority )
            {
                worst_priority = scheduler_ptr->current_jobs_on_cores[i]->priority;
                arrival_time = scheduler_ptr->current_jobs_on_cores[i]->arrival_time;
                core = i;
            }
            else if ( scheduler_ptr->current_jobs_on_cores[i]->priority == worst_priority )
            {
                if( scheduler_ptr->current_jobs_on_cores[i]->arrival_time >  arrival_time )
                {
                    arrival_time = scheduler_ptr->current_jobs_on_cores[i]->arrival_time;
                    core = i;
                }
            }

        }
    }
    return core;
}

/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
    job_t* job = calloc( 1, sizeof(job_t));
    job->pid = job_number;
    job->arrival_time = time;
    job->priority = priority;
    job->total_time_needed = running_time;
    job->used_time = 0;
    job->last_start_time = 0;
    job->job_response_time = 0;
    const scheme_t scheme = scheduler_ptr->scheduler_scheme;
    //either schedule it or place it in the queue;

    int first_core = idleCore();
    if( first_core != -1 )
    {
        scheduler_ptr->current_jobs_on_cores[first_core] = job;
        job->last_start_time = time;
        return first_core;
    }


    if( scheme == PSJF )
    {
        int longest_job = findLongestRemainingJob();
        int longest_job_last_remaining_time = remainingTime( scheduler_ptr->current_jobs_on_cores[longest_job] );
        int longest_job_current_remaining_time = longest_job_last_remaining_time - ( time - scheduler_ptr->current_jobs_on_cores[longest_job]->last_start_time );

        if( longest_job_current_remaining_time <=  job->total_time_needed )
        {
            // all jobs on cores have lower times, thus higher priority, add this one to queue
            priqueue_offer ( &scheduler_ptr->job_queue, job );
            return -1;
        }
        else
        {
            //remove old
            job_t *old_job = scheduler_ptr->current_jobs_on_cores[longest_job];
            //log how much time it used
            old_job->used_time += (time - old_job->last_start_time );

            //replace with new
            job->last_start_time = time;
            scheduler_ptr->current_jobs_on_cores[longest_job] = job;

            //push old to queue
            priqueue_offer ( &scheduler_ptr->job_queue, old_job );
            return longest_job;
        }

    }
    else if( scheme == PPRI )
    {
        int worst_priority_idx =  findWorstPriorityJob();

        if( scheduler_ptr->current_jobs_on_cores[worst_priority_idx]->priority <= job->priority )
        {
            // all jobs on cores have lower times, thus higher priority, add this one to queue
            priqueue_offer ( &scheduler_ptr->job_queue, job );
            return -1;
        }
        else
        {
            //remove old
            job_t *old_job = scheduler_ptr->current_jobs_on_cores[worst_priority_idx];
            //log how much time it used
            old_job->used_time += (time - old_job->last_start_time );

            //replace with new
            job->last_start_time = time;
            scheduler_ptr->current_jobs_on_cores[worst_priority_idx] = job;

            //push old to queue
            priqueue_offer ( &scheduler_ptr->job_queue, old_job );
            return worst_priority_idx;
        }

    }
    else if( scheme == RR || scheme == PRI || scheme == FCFS || scheme == SJF )
    {
    	if( first_core == -1 )
    	{
    		priqueue_offer ( &scheduler_ptr->job_queue, job);
    	}
    }

    return -1;
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
    job_t *old_job = scheduler_ptr->current_jobs_on_cores[core_id];
    scheduler_ptr->current_jobs_on_cores[core_id] = NULL;

    scheduler_ptr->total_jobs_count++;
	scheduler_ptr->total_wait_time += (time - old_job->arrival_time - old_job->total_time_needed);
	scheduler_ptr->total_turn_around_time += (time - old_job->arrival_time);
    scheduler_ptr->total_response_time += old_job->job_response_time;

    free( old_job );

    // Check for a new job
    job_t *new_job = priqueue_poll( &scheduler_ptr->job_queue );
	if( !new_job )
	{
		return -1;
	}
    else
	{
		if( 0 == new_job->used_time )
		{
            // this is the first time we have scheduled it, update response
            // time as such.
			new_job->job_response_time = ( time - new_job->arrival_time );
		}
        // place it on a core, and update its last start time
		scheduler_ptr->current_jobs_on_cores[core_id] = new_job;
		new_job->last_start_time = time;

		return new_job->pid;
	}
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	job_t *old = scheduler_ptr->current_jobs_on_cores[core_id];
    old->used_time += ( time - old->last_start_time );
    priqueue_offer( &scheduler_ptr->job_queue, old);

    job_t *new = priqueue_poll( &scheduler_ptr->job_queue );
    if( !new )
    {
        return -1;
    }
    else
    {
        if( 0 == new->used_time )
        {
            // this is the first time we have scheduled it, update response
            // time as such.
            new->job_response_time = ( time - new->arrival_time );
        }
        new->last_start_time = time;
        scheduler_ptr->current_jobs_on_cores[core_id] = new;

        return new->pid;
    }
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
    if(scheduler_ptr->total_jobs_count == 0)
		return 0;
	else
		return (float)scheduler_ptr->total_wait_time/(float)scheduler_ptr->total_jobs_count;
}



/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
    if(scheduler_ptr->total_jobs_count == 0)
    	return 0.0;
    else
        return (float)scheduler_ptr->total_turn_around_time/(float)scheduler_ptr->total_jobs_count;

}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
    if(scheduler_ptr->total_jobs_count == 0)
		return 0.0;
	else
		return (float)scheduler_ptr->total_response_time/(float)scheduler_ptr->total_jobs_count;
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
    priqueue_destroy( &scheduler_ptr->job_queue );
	free( scheduler_ptr->current_jobs_on_cores );
	free( scheduler_ptr );
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
