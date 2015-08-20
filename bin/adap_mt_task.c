/* based_mt_task.c -- A basic multi-threaded real-time task skeleton. 
 *
 * This (by itself useless) task demos how to setup a multi-threaded LITMUS^RT
 * real-time task. Familiarity with the single threaded example (base_task.c)
 * is assumed.
 *
 * Currently, liblitmus still lacks automated support for real-time
 * tasks, but internaly it is thread-safe, and thus can be used together
 * with pthreads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

/* Include gettid() */
#include <sys/types.h>

/* Include threading support. */
#include <pthread.h>

#include "whisper.h"
#include <math.h> 

/* Include the LITMUS^RT API.*/
#include "litmus.h"

#include "event_tracker.h"
#include "feedback.h"

#include <time.h>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>


// TODO: Increased Period. Maybey that will solve my problem 
#define PERIOD            40 // 40 gives us 25 ticks per second
//#define PERIOD            2048
#define RELATIVE_DEADLINE PERIOD 
#define EXEC_COST         2

#define R_ARRAY_SIZE_1	  101
#define R_ARRAY_SIZE_2	  103

/* Let's create 10 threads in the example, 
 * for a total utilization of 1.
 */
//#define NUM_THREADS      96 
#define NUM_THREADS      100 
 
//Change to 1 if clustered
#define CLUSTERED 1
#define NUM_CLUSTER 2  



struct service_levels_struct {
	pid_t task_id;
	unsigned int service_level;
};


struct task_id_to_index_map {
	pid_t task_id;
	unsigned int index;
	struct task_id_to_index_map* next;
};

/* The information passed to each thread. Could be anything. */
struct thread_context {
	int id;
	struct service_levels_struct* service_levels_array;
};


struct scraper_context {
	int id;
	int cpus; 
	int tasks; 
	char* kill_switch;
	struct core_file_struct* traceFiles; 
	struct service_levels_struct* service_levels_array;
};



/* The real-time thread program. Doesn't have to be the same for
 * all threads. Here, we only have one that will invoke job().
 */
void* rt_thread(void *tcontext);

/* Declare the periodically invoked job. 
 * Returns 1 -> task should exit.
 *         0 -> task should continue.
 */
int job(int id, struct rt_task param, micSpeakerStruct* ms,  double rArray1[], double rArray2[], int ticks);

void* scraper_thread(void* s_context);

void init_map(struct task_id_to_index_map map[], unsigned int number_tasks);
int get_index_of_task_id(struct task_id_to_index_map map[], unsigned int number_tasks, pid_t task_id);
void set_index_of_task_id(struct task_id_to_index_map map[], unsigned int number_tasks, pid_t task_id, unsigned int index );
void delete_map(struct task_id_to_index_map map[], unsigned int number_tasks);

/* Catch errors.
 */
#define CALL( exp ) do { \
		int ret; \
		ret = exp; \
		if (ret != 0) \
			fprintf(stderr, "%s failed: %m\n", #exp);\
		else \
			fprintf(stderr, "%s ok.\n", #exp); \
	} while (0)


/* Basic setup is the same as in the single-threaded example. However, 
 * we do some thread initiliazation first before invoking the job.
 */
int main(int argc, char** argv)
{
//TODO: Remove comments up here as well
//	int i;

	int numberOfMics = 4;
	int numberOfSpeakers = NUM_THREADS/numberOfMics;

	//struct event_struct* data;

	int i = 0;
	//size_t dataRead;

	

	struct thread_context 	ctx[NUM_THREADS];
	pthread_t             	task[NUM_THREADS];
	pthread_t*				scraper = (pthread_t*) malloc(sizeof(pthread_t));
	
	struct core_file_struct* traceFiles;
	int number_of_Cores = 24;
	struct scraper_context* s_context = (struct scraper_context*)malloc(sizeof(struct scraper_context));
	char * kill_switch = (char* )malloc(sizeof(char));
	struct service_levels_struct* service_levels_array = (struct service_levels_struct*) malloc(sizeof(struct service_levels_struct)*(NUM_THREADS+1));
	
	
	
	if ((s_context==NULL)||(kill_switch==NULL) || (service_levels_array== NULL)){
		fprintf(stderr, "Error : out of memory- %s\n", strerror(errno));
		return 1;
	}
	
	(*kill_switch) = 1;
	/* The task is in background mode upon startup. */		

	for (i = 0; i < (NUM_THREADS+1); i++){
		service_levels_array[i].task_id=0;
		service_levels_array[i].service_level=0;
	}
	/*****
	 * 1) Command line paramter parsing would be done here.
	 */


       
	/*****
	 * 2) Work environment (e.g., global data structures, file data, etc.) would
	 *    be setup here.
	 */
	 
	traceFiles=loadFiles(number_of_Cores);
	if (traceFiles == NULL){
		fprintf(stderr, "Error : Couldn't load trace files - %s\n", strerror(errno));
		return 1;
	}

	

	
	


	//micSpeakerStruct* ms1;
	//OccludingPointsStruct* ops;

	//initWhisperRoom(3, 2, 8, 4, 1, 2000000, 800000, 1.2, 100000, 100);
	// 1.3 is normal human walking speed 
	// 1000 makes each whisper unit 1 mm
	// 25 gives us 25 updates per second
	//(double alpha, double beta, int numSpeakers, int numMic, int occluding, long int occludingSize,
	//				long int side, long int radius, double speedInMetersPerSecond, long int unitsPerMeter, 
	//				long int ticksPerSecond)
	initWhisperRoom(5, 1.2, numberOfSpeakers, numberOfMics, 0, 2500, 20000, 9000, 1.3, 1000, 25);
	//initWhisperRoom(1, 1.2, numberOfSpeakers, numberOfMics, 0, 2500, 20000, 9000, 1.3, 1000, 25);

	
	addNoise(20, 10, 1.1);
	addNoise(40, 10, 1.5);
	//ms1 = constructSpeakerMicPairByNumber(2*8-1);
/*	for(j = 0;j<4*8;j++){
		 ms = constructSpeakerMicPairByNumber(j);
	}*/
	
	//ops = (OccludingPointsStruct*)malloc(sizeof(OccludingPointsStruct));



	/*****
	 * 3) Initialize LITMUS^RT.
	 *    Task parameters will be specified per thread.
	 */
	init_litmus();


	/***** 
	 * 4) Launch threads.
	 */
//TODO: Remove the comments. They are here while we test setting up the system
	for (i = 0; i < NUM_THREADS; i++) {
		ctx[i].id = i;
		ctx[i].service_levels_array = service_levels_array;
		pthread_create(task + i, NULL, rt_thread, (void *) (ctx + i));
	}
	
	
	s_context->id = NUM_THREADS;
	s_context->cpus = number_of_Cores;
	s_context->tasks = NUM_THREADS;
	s_context->kill_switch = kill_switch;
	s_context->traceFiles = traceFiles;
	s_context->service_levels_array = service_levels_array;
	
	pthread_create(scraper, NULL, scraper_thread, s_context);
	
// 	struct scraper_context {
// 	int id;
// 	int cpus; 
// 	int tasks; 
// 	char* kill_switch;
// 	struct core_file_struct* traceFiles; 
// 	struct service_levels_struct* service_levels_array;
// };


	
	/*****
	 * 5) Wait for RT threads to terminate.
	 */
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(task[i], NULL);
	
	//All tasks are done, it's time for the scraper to go now.
	(*kill_switch) = 0;
	pthread_join( (*scraper), NULL);
	
	free(kill_switch);
	free(s_context);

	free(scraper);
	/***** 
	 * 6) Clean up, maybe print results and stats, and exit.
	 */
	 
	for (i = 0; i< number_of_Cores;i++){
		printf("Closing file %d\n", i);
		fclose(traceFiles[i].file);
	}
	free(traceFiles);
	return 0;
}



/* A real-time thread is very similar to the main function of a single-threaded
 * real-time app. Notice, that init_rt_thread() is called to initialized per-thread
 * data structures of the LITMUS^RT user space libary.
 */
void* rt_thread(void *tcontext)
{
	//int do_exit;
	micSpeakerStruct* ms;
	struct thread_context *ctx = (struct thread_context *) tcontext;
	struct rt_task param;
	double randomValues1[R_ARRAY_SIZE_1];
	double randomValues2[R_ARRAY_SIZE_2];
	int i;
	int k;
	
	//Added:
	int numberClusters = NUM_CLUSTER;
	int cluster = ctx->id%numberClusters; 
	int ret;
	struct timespec last_time;
	struct timespec after_job;
	struct timespec current_time;
	int totalTicks;
	int currentTicks;
	double last_time_in_seconds;
	double current_time_in_seconds;
	double after_job_time_in_seconds;
	double theWeight; 
	double totalWeight = 0;

	pid_t my_id = gettid();
	
	//Store your id in the global array
	ctx->service_levels_array[ctx->id].task_id = my_id;
	


	
	for (i = 0; i < R_ARRAY_SIZE_1 ; i++) {
		randomValues1[i] = (rand()%5000)/5000.0;
	}
	
	for (i = 0; i < R_ARRAY_SIZE_2 ; i++) {
		randomValues2[i] = (rand()%5000)/2500.0-1;
	}
	
	ms = constructSpeakerMicPairByNumber(ctx->id);

	/* Set up task parameters */
	//added.....
	//this works for tasks that have migration
	//this works for tasks that have migration
	// This and the other migrate_to_domain work beneath are necessary to get the system 
	// working for clustered EDF
	if(CLUSTERED==1){	
		ret = be_migrate_to_domain(cluster); 
		if (ret < 0){
			printf("Couldn't migrate\n");
		}
	}
	//......
	
	init_rt_task_param(&param);
	param.exec_cost = ms2ns(EXEC_COST);
	param.period = ms2ns(PERIOD);
	param.relative_deadline = ms2ns(RELATIVE_DEADLINE);

	/* What to do in the case of budget overruns? */
	param.budget_policy = NO_ENFORCEMENT;

	/* The task class parameter is ignored by most plugins. */
	param.cls = RT_CLASS_SOFT;

	/* The priority parameter is only used by fixed-priority plugins. */
	param.priority = LITMUS_LOWEST_PRIORITY;

	/* Make presence visible. */
// 	printf("RT Thread %d active.\n", ctx->id);
	
	// 
// 	//The service level information is know by  the task and the system. 
// 	param.service_levels =(struct rt_service_level*)malloc(sizeof(struct rt_service_level)*5); 
// 	param.service_levels[0].relative_work = 1; 
// 	param.service_levels[0].quality_of_service = 1;
// 	param.service_levels[0].service_level_number = 0;
// 	param.service_levels[0].service_level_period = ms2ns(PERIOD);
// 
// 	param.service_levels[1].relative_work = 2;
// 	param.service_levels[1].quality_of_service = 2;
// 	param.service_levels[1].service_level_number = 1;
// 	param.service_levels[1].service_level_period = ms2ns(PERIOD);
// 
// 	param.service_levels[2].relative_work = 15;
// 	param.service_levels[2].quality_of_service = 15;
// 	param.service_levels[2].service_level_number = 2;
// 	param.service_levels[2].service_level_period = ms2ns(PERIOD);
// 
// 	param.service_levels[3].relative_work = 16;
// 	param.service_levels[3].quality_of_service = 16;  
// 	param.service_levels[3].service_level_number = 3; 
// 	param.service_levels[3].service_level_period = ms2ns(PERIOD);

// 	printf("Service level 0 %llu\n", param.service_levels[0].service_level_period);
// 	printf("Service level 1 %llu\n", param.service_levels[1].service_level_period);
// 	printf("Service level 2 %llu\n", param.service_levels[2].service_level_period);
// 	printf("Service level 3 %llu\n", param.service_levels[3].service_level_period);
	
	//added.....
// 	if(CLUSTERED==1){
// 		param.cpu = domain_to_first_cpu(cluster);
// 	}
	//.....
	
	/*****
	 * 1) Initialize real-time settings.
	 */
	CALL( init_rt_thread() );

	/* To specify a partition, do
	 *
	 * param.cpu = CPU;
	 * be_migrate_to(CPU);
	 *
	 * where CPU ranges from 0 to "Number of CPUs" - 1 before calling
	 * set_rt_task_param().
	 */
	CALL( set_rt_task_param(my_id, &param) );

	/*****
	 * 2) Transition to real-time mode.
	 */
	CALL( task_mode(LITMUS_RT_TASK) );

	/* The task is now executing as a real-time task if the call didn't fail. 
	 */



	/*****
	 * 3) Invoke real-time jobs.
	 */
	clock_gettime(CLOCK_MONOTONIC, &last_time);
	last_time_in_seconds = ((last_time.tv_sec) + (last_time.tv_nsec)*0.000000001);
	totalTicks = 0;
	//Was 6000 make 60 for good reason
	for(k=0;k<600;k++){
		/* Wait until the next job is released. */
		sleep_next_period();
		
		
		/* Get tick count */
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		current_time_in_seconds = ((current_time.tv_sec) + (current_time.tv_nsec)*0.000000001);
		currentTicks = (int)((current_time_in_seconds-last_time_in_seconds)*WHISPER_TICS_PER_SECOND)-totalTicks;
		totalTicks = (int)((current_time_in_seconds-last_time_in_seconds)*WHISPER_TICS_PER_SECOND);
		
		/* Invoke job. */
		job(ctx->id, param, ms,randomValues1, randomValues2, currentTicks);
		clock_gettime(CLOCK_MONOTONIC, &after_job);
		
		after_job_time_in_seconds = ((after_job.tv_sec) + (after_job.tv_nsec)*0.000000001);
		theWeight = (after_job_time_in_seconds-current_time_in_seconds)/0.04;
		totalWeight +=theWeight;
		
		
	
	
		//If we haven't made any progress since the last go around, then don't change the last
		//time otherwise. we won't move anywhere
// 		if (ticks !=0){
// 			clock_gettime(CLOCK_MONOTONIC, &last_time);
// 		}
		//printf("Job Number %d, my id %d\n", k, ctx->id);
// 		if (k%5000==0){
// 			avgWeight = totalWeight/k;
// 
// 			//printf("Job Time in seconds %f\n", (after_job_time_in_seconds-current_time_in_seconds)); 
// 			//printf("Difference in time %f, ticks %d\n", (current_time_in_seconds-last_time_in_seconds), currentTicks); 
// 			printf("Time %f, Thread %d,average weight is %f, currentWeight%f,\n",(current_time_in_seconds-last_time_in_seconds), ctx->id,avgWeight,theWeight ); 
// 		} 
	}// while (!do_exit);


	
	/*****
	 * 4) Transition to background mode.
	 */
	CALL( task_mode(BACKGROUND_TASK) );


	return NULL;
}



int job(int id, struct rt_task param,  micSpeakerStruct* ms, double rArray1[], double rArray2[], int ticks) 
{
	/* Do real-time calculation. */
	long int i =0;
	int rIndex1 = 0;
	int rIndex2 = 0;
	int total=0;
	int numberOfOperations;
	int relativeWorkFactor = 1;
	unsigned int myServiceLevel = 0;
	// myControlPage->service_level;
//	double micDistance;

	//myControlPage->service_level+=id;
//	myServiceLevel = myControlPage->service_level;
	//printf("**Service Level %u of thread %d, period %llu\n",myServiceLevel, id,param.service_levels[myServiceLevel].service_level_period);
	if ((myServiceLevel >=0) && (myServiceLevel <= 3)) {
		relativeWorkFactor = 1; 
		//param.service_levels[myServiceLevel].relative_work;
		//printf("**Service Level %u, relative work %d, of thread %d\n",myServiceLevel,relativeWorkFactor, id);
	} else {
		//printf("Error Service level %u  too high %d\n",myServiceLevel, id);
	}
	
	/* Don't exit. */
	
	updatePosition(ms, ticks);
//	micDistance = getMicSpeakerDistanceInMeters(ms);
// 	if(id==0){
// 		printf("For thread %d, the distance is %f\n", id,micDistance);
// 		printf("\t\tFor thread %d, the total operations is %d\n", id, getNumberOfOperations(ms));
// 	}
	//Increased the number of iterations 
	//TODO: 2014- move increase into whisper
	//numberOfOperations = getNumberOfOperations(ms)*14000;
	
	//For 96 tasks 
	//   numberOfOperations = getNumberOfOperations(ms)*relativeWorkFactor*2;
	numberOfOperations = (int)(getNumberOfOperations(ms)*relativeWorkFactor*2/2);

	total = 0;
	for (i=0;i<numberOfOperations;i++){
		
		if (rIndex1 >= R_ARRAY_SIZE_1 ){
			rIndex1 = 0;
		}
		
		if (rIndex2 >= R_ARRAY_SIZE_2){
			rIndex2 = 0;
		}
		total += rArray1[rIndex1] * rArray2[rIndex2];
		rIndex1++;
		rIndex2++;
//		printf("i %ld\n", i);
	}
	
	if(total==RAND_MAX){
		printf("Just here to make sure total isn't optimized away\n");
	}
	return 0;
}

void* scraper_thread(void* s_context) {

	size_t events_to_read = 8;
	size_t dataRead;
	size_t innerCount;
	struct event_struct * data;
	struct event_struct * tempEvent;
	struct execution_time_struct execution_time_array[NUM_THREADS+1];
	struct execution_time_struct* temp_et_struct;
	struct task_id_to_index_map index_map[NUM_THREADS+1];
	int map_size = NUM_THREADS+1;
	
	struct scraper_context *ctx = (struct scraper_context *) s_context;
	struct rt_task param;
	pid_t my_id = gettid();
	int i,k;
	int index_of_task;
	const double alphaValue = 0.102;
	const double betaValue = 0.30345;
	//long long estimated_execution_time;

	init_rt_task_param(&param);
	param.exec_cost = ms2ns(EXEC_COST);
	param.period = ms2ns(PERIOD);
	param.relative_deadline = ms2ns(RELATIVE_DEADLINE);

	/* What to do in the case of budget overruns? */
	param.budget_policy = NO_ENFORCEMENT;

	/* The task class parameter is ignored by most plugins. */
	param.cls = RT_CLASS_SOFT;

	/* The priority parameter is only used by fixed-priority plugins. */
	param.priority = LITMUS_LOWEST_PRIORITY;


	
	/*****
	 * 1) Initialize real-time settings.
	 */
	CALL( init_rt_thread() );

	CALL( set_rt_task_param(my_id, &param) );

	/*****
	 * 2) Transition to real-time mode.
	 */
	CALL( task_mode(LITMUS_RT_TASK) );

	/* The task is now executing as a real-time task if the call didn't fail. 
	 */
	 
	ctx->service_levels_array[map_size-1].task_id = my_id;
	
	init_map(index_map, map_size);
	
	for(k =0; k < map_size;k++){
		execution_time_array[k].cumulative_difference=0;
		execution_time_array[k].current_difference=0;
		execution_time_array[k].actual_execution=0;
		execution_time_array[k].estimated_execution=0;	
	}

	//We dont' want this task executing until all tasks have intialized the structure
	
	k = 0;
	// We include the kill switch here so that way if there is a problem, then we won't keep trying it's over
	// 
	while ( (k < map_size) &&  (*ctx->kill_switch) != 0) {
		//If it's greater than 0, then it's been assigned
		if (ctx->service_levels_array[k].task_id > 0) {
			//Add it to the map
			set_index_of_task_id(index_map, map_size,ctx->service_levels_array[k].task_id,k);
			k++;
		} else {
			// must not have been set. Go to sleep, try again later
			sleep_next_period();
		}
	}
	

	/*****
	 * 3) Invoke real-time jobs.
	 */
	data = (struct event_struct*)malloc(events_to_read*sizeof(struct event_struct));
	do{
		for(i = 0; i < ctx->cpus; i++){
			dataRead = read_trace_record(ctx->traceFiles[i].file, data, events_to_read); 
			if (dataRead != 0) {
				for(innerCount = 0; innerCount < dataRead; innerCount++){
					tempEvent = data+(sizeof(struct event_struct)*innerCount);
					index_of_task = get_index_of_task_id(index_map, map_size,tempEvent->task_id);

					if (index_of_task >= 0) {
						temp_et_struct = &(execution_time_array[index_of_task]);
						/* The alpha and beta values 0.102 and 0.30345 are the a and b values that are calculated from
						 * Aaron Block's dissertation referenced on pages 293 (the experimental values for
						 * a and c) and the relationship of a,b,c is given on page 253 just below (6.2)
						 */
						 printf("For task %hi Previous estimated %lli\n",tempEvent->task_id,temp_et_struct->estimated_execution);
						 printf("For task %hi previous actual    %llu\n\n", tempEvent->task_id,tempEvent->exec_time);
						 calc_estimated(temp_et_struct, tempEvent->exec_time, alphaValue, betaValue);
						 

					}
				}
			}
		}

		/* Wait until the next job is released. */
		sleep_next_period();
	} while ((*ctx->kill_switch) != 0);
	free(data);
	delete_map(index_map, map_size);

	
	/*****
	 * 4) Transition to background mode.
	 */
	CALL( task_mode(BACKGROUND_TASK) );


	return NULL;
	
}

void init_map(struct task_id_to_index_map map[], unsigned int number_tasks){
	unsigned int i;
	for (i = 0; i < number_tasks;i++){
		map[i].task_id=0;
		map[i].index=0;
		map[i].next= NULL;
	}
}

int get_index_of_task_id(struct task_id_to_index_map map[], unsigned int number_tasks, pid_t task_id){
	int hash_id = task_id % number_tasks;
	struct task_id_to_index_map* current_task;
	if (map == NULL){
		return -1;
	}
	if (map[hash_id].task_id == task_id) {
		return  map[hash_id].index;
	} else {
		current_task = map[hash_id].next; 
		//If there are multiple collisions, then find the correct value
		while(current_task!=NULL){
			if (current_task->task_id == task_id) {
				return  current_task->index;
			}
			current_task = current_task->next;
		}
		return -1;
	} 		
}


void set_index_of_task_id(struct task_id_to_index_map map[], unsigned int number_tasks, pid_t task_id, unsigned int index ){
	int hash_id = task_id % number_tasks;
	struct task_id_to_index_map* current_task;
	struct task_id_to_index_map* last_task;
	if (map == NULL){
		return;
	}
	//If the spot is empty, then just make it here
	if (map[hash_id].task_id == 0){
		map[hash_id].task_id = task_id;
		map[hash_id].index = index;
		map[hash_id].next = NULL;
		return;
	}
	
	if (map[hash_id].task_id == task_id) {
		// Already in. no need to insert
		return;
	} else {
		if (map[hash_id].next == NULL) {
			//only one element, let's add it here
			map[hash_id].next = (struct task_id_to_index_map*) malloc(sizeof(struct task_id_to_index_map));
			map[hash_id].next->task_id = task_id;
			map[hash_id].next->index = index;
			map[hash_id].next->next = NULL;
			return;
		} 
		last_task = map[hash_id].next;
		current_task = last_task->next;  
		
		//If there are multiple collisions, then find the correct value
		while(current_task!=NULL){
			if (current_task->task_id == task_id) {
				//Already in. don't add it
				return;
			} 
			last_task = current_task;
			current_task = last_task->next; 
		}
		//add it here and then return
		last_task->next = (struct task_id_to_index_map*) malloc(sizeof(struct task_id_to_index_map));
		last_task->next->task_id = task_id;
		last_task->next->index = index;
		last_task->next->next = NULL;
		return;
	} 		
}

//recursively free all pages
void delete_map_helper(struct task_id_to_index_map* current){
	if (current->next != NULL){
		delete_map_helper(current->next);
	}
	free(current);
}

void delete_map(struct task_id_to_index_map map[], unsigned int number_tasks){
	int i;
	for(i=0;i<number_tasks;i++){
		if (map[i].next!=NULL){
			delete_map_helper(map[i].next);
		}
	}
}
	
 
