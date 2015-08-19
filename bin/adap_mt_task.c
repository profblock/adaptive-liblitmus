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

#include <time.h>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <sys/fcntl.h>

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

#define ENABLE_CMD  0x0
#define DISABLE_CMD	0x1

// event ID for schedcat job completions
#define ST_JOB_COMPLETION 506

//How much data in a binary completion record?
#define ST_JOB_COMPLETION_LEN 24 // bytes

/* The information passed to each thread. Could be anything. */
struct thread_context {
	int id;
};


struct Core_file_struct {
	FILE* file;
	int coreID;
};

struct Event_struct {
	unsigned char event_id;
    unsigned char cpu;
	unsigned short task_id;
	unsigned int job_id;
	unsigned long long when;
    unsigned long long exec_time;
    unsigned char was_forced;
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



struct Core_file_struct* traceFiles;
int number_of_Cores = 24;

void printEventStruct(struct Event_struct* event);

int convert_data_stream_to_event_struct(void* data, size_t offset, struct Event_struct* event);

struct Core_file_struct* loadFiles(int numberOfCores);

int matchesFileStart(char* fileName, char* target);

int turn_events_on(FILE* file, int event_id);

int turn_events_off(FILE* file, int event_id);

int turn_non_blocking(FILE* file);

size_t read_trace_record(FILE* file, struct Event_struct* data, size_t number_of_records_to_read); 


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

	//struct Event_struct* data;

	int i = 0;
	//size_t dataRead;

	

	struct thread_context ctx[NUM_THREADS];
	pthread_t             task[NUM_THREADS];

	/* The task is in background mode upon startup. */		


	/*****
	 * 1) Command line paramter parsing would be done here.
	 */


       
	/*****
	 * 2) Work environment (e.g., global data structures, file data, etc.) would
	 *    be setup here.
	 */
	 
	traceFiles=loadFiles(number_of_Cores);
	if (traceFiles == NULL){
		printf("Couldn't load trace files\n");
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
		pthread_create(task + i, NULL, rt_thread, (void *) (ctx + i));
	}

	
	/*****
	 * 5) Wait for RT threads to terminate.
	 */
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(task[i], NULL);
	

	/***** 
	 * 6) Clean up, maybe print results and stats, and exit.
	 */
	 
	for (i = 0; i< number_of_Cores;i++){
		printf("Closing file %d\n", i);
		fclose(traceFiles[i].file);
	}
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
	size_t innerCount;
	
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
	//double avgWeight = 0;
	
	size_t events_to_read = 8;
	size_t dataRead;
	struct Event_struct * data;
	struct Event_struct * tempEvent;
	


	
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
	CALL( set_rt_task_param(gettid(), &param) );

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
		
		
		if ((k%100==0) && (k>0)){
			data = (struct Event_struct*)malloc(events_to_read*sizeof(struct Event_struct));
			dataRead = read_trace_record(traceFiles[cluster].file, data, events_to_read); 
			if (dataRead == 0) {
				printf("nothing read\n");
			} else {
				for(innerCount = 0; innerCount < dataRead; innerCount++){
					tempEvent = data+(sizeof(struct Event_struct)*innerCount);
					printEventStruct(tempEvent);
				}
			}
			free(data);
		}
	
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
//	struct control_page* myControlPage = get_ctrl_page();
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


// *********************************
// METHODS FOR LOADING THE TRACING INFORMATION 
// **********************************
struct Core_file_struct* loadFiles(int numberOfCores){
	DIR* fileDirectory; 
	//OPen the directory
	struct dirent* fileStruct;
	char* targetDirectory = "/dev/litmus/";
	char* targetFileName = "sched_trace\0";
	int fileNumber =-1;
	struct Core_file_struct*  array_of_files = (struct Core_file_struct*) malloc(sizeof(struct Core_file_struct)*numberOfCores);
	char fileName[1024]; //We assume that filenames are at most 1024 charcters
// 	char** fileNames = (char**) malloc(sizeof(char*)*numberOfCores); 
// 	char** tmpHolder;
	if (array_of_files == 0) {
		fprintf(stderr, "Error :Out of memmory\n");
		return NULL;
	}
	printf("Loading Files\n");
	fileDirectory = opendir(targetDirectory);
	if (NULL == fileDirectory) {
		printf("Error couldn't open\n");
		fprintf(stderr, "Error : Failed to open directory - %s\n", strerror(errno));
		free(array_of_files);
		return NULL;
	} 
	
	while ((fileStruct = readdir(fileDirectory))){		
		if (!strcmp (fileStruct->d_name, "."))
			continue;
		if (!strcmp (fileStruct->d_name, ".."))
			continue;
		fileNumber = matchesFileStart(fileStruct->d_name,targetFileName);

		if ((fileNumber >= 0) && (fileNumber < numberOfCores)) {
			printf("Let's open it! is %d\n", fileNumber);
			printf("Nnumber of cores is %d\n", numberOfCores);
			snprintf(fileName, sizeof(fileName), "%s%s",targetDirectory,fileStruct->d_name);
			array_of_files[fileNumber].coreID = fileNumber;
			array_of_files[fileNumber].file = fopen(fileName, "rb");
			if (array_of_files[fileNumber].file == NULL)
			{
				printf("couldn't open %d ... Womp Womp\n", fileNumber);
				fprintf(stderr, "Error : Failed to open entry file - %s\n", strerror(errno));
				free(array_of_files);
				return NULL;
			}	
			//Open with no buffer
			setvbuf(array_of_files[fileNumber].file, NULL, _IONBF, 0);
			//fclose(array_of_files[fileNumber].file);		
			
			turn_events_on(array_of_files[fileNumber].file,ST_JOB_COMPLETION);
			
			turn_non_blocking(array_of_files[fileNumber].file);

		} else if (fileNumber >= numberOfCores) {
			printf("Too many files\n");
			fprintf(stderr, "Error : Too many files - %s\n", strerror(errno));
			free(array_of_files);
			return NULL;
		}
	}
	printf("directory open!\n");
	return array_of_files;
}

//Note both fileName and target must be null terminated strings
//Assumes that all files end in a number and this returns the number
int matchesFileStart(char* fileName, char* target) {
	char currentFileNameChar;
	char currentTargetChar;
	int index = 0 ;
	int secondLoopIndex = 0;
	int returnNumber = -1;
	char remainingString[1024];
	currentFileNameChar = fileName[index];
	currentTargetChar = target[index];
	

	
	while (currentFileNameChar != 0 && currentTargetChar != 0){
		//If the characters are different, then return 0
		if (currentFileNameChar != currentTargetChar)
			return -1;
		index++;
		currentFileNameChar = fileName[index];
		currentTargetChar = target[index];
	} 
	while (currentFileNameChar != 0) {
		remainingString[secondLoopIndex] = currentFileNameChar;
		secondLoopIndex++;
		currentFileNameChar = fileName[index+secondLoopIndex];
	}
	//Gotta terminate with a 0
	remainingString[secondLoopIndex] = 0;
	returnNumber = atoi(remainingString);
	if (currentTargetChar == 0) 
		return returnNumber;
	else
		return -1;
}

int turn_events_on(FILE* file, int event_id) {
	//Get the file descriptor for a file
	int fd = fileno(file);
	if (fd < 0) {
		printf("Error, file not found\n");
		fprintf(stderr, "Error : file not found - %s\n", strerror(errno));
		return -1;
	}
	return ioctl(fd,ENABLE_CMD, event_id);
}


int turn_non_blocking(FILE* file){
	//Get the file descriptor for a file
	int fd = fileno(file);
	int flags;
	if (fd < 0) {
		printf("Error, file not found\n");
		fprintf(stderr, "Error : file not found - %s\n", strerror(errno));
		return -1;
	}
	//Turn on non blocking flags
	flags = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int turn_events_off(FILE* file, int event_id) {
	//Get the file descriptor for a file
	int fd = fileno(file);
	if (fd < 0) {
		printf("Error, file not found\n");
		fprintf(stderr, "Error : file not found - %s\n", strerror(errno));
		return -1;
	}	

	return ioctl(fd,DISABLE_CMD, event_id);
}

size_t read_trace_record(FILE* file, struct Event_struct* data, size_t number_of_records_to_read) {
	size_t recordsRead; 
	size_t i;
	size_t offset=0;
	struct Event_struct* eventStructTempPointer;
	size_t records_actually_read;
	void* tempStore = (void*)malloc(number_of_records_to_read*ST_JOB_COMPLETION_LEN);
	if (tempStore == 0) {
		fprintf(stderr, "Error : Out of memory- %s\n", strerror(errno));
		return 0;
	}
	recordsRead = fread(tempStore, ST_JOB_COMPLETION_LEN, number_of_records_to_read, file);
	if (recordsRead == 0){
		fprintf(stderr, "Error : couldn't read any data\n");
		free(tempStore);
		return 0;
	}

	
	//Convert the raw data into an array of even structs 
	records_actually_read = recordsRead;
	eventStructTempPointer = data;
	for (i=0; i < records_actually_read; i++){
		convert_data_stream_to_event_struct(tempStore, offset, eventStructTempPointer);
		offset+=ST_JOB_COMPLETION_LEN;
		eventStructTempPointer +=sizeof(struct Event_struct);
	}
	free(tempStore);
	return records_actually_read;
	
}

int convert_data_stream_to_event_struct(void* data, size_t offset, struct Event_struct* event){
	unsigned char* event_id;
    unsigned char* cpu;
	unsigned short* task_id;
	unsigned int* job_id;
	unsigned long long* when;
    unsigned long long* exec_time;
    
	if ((data == 0) || ( event == 0)){
		fprintf(stderr, "Error : data or event is null \n");
		return 1;
	}
	//No begins struct data arithmatic 
	event_id = (unsigned char*) (data+offset);
    cpu = (unsigned char*)(event_id+sizeof(unsigned char));
	task_id = (unsigned short*)(cpu+sizeof(unsigned char));
	job_id = (unsigned int*)(data+offset+sizeof(unsigned char)+sizeof(unsigned char)+sizeof(unsigned short)); //(task_id+sizeof(unsigned short));
	when = (unsigned long long *)(data+offset+sizeof(unsigned char)+sizeof(unsigned char)+sizeof(unsigned short)+sizeof(unsigned int));
    exec_time = (unsigned long long *)(data+offset+sizeof(unsigned char)+sizeof(unsigned char)+sizeof(unsigned short)+sizeof(unsigned int)+sizeof(unsigned long long));
    
    event->event_id = *event_id;
    event->cpu = *cpu;
	event->task_id = *task_id;
	event->job_id = *job_id;
	event->when = *when;
    event->exec_time = *exec_time;
    //Adjust the was_forced and exec_time 
    event->was_forced = event->exec_time & 0x1;
 	event->exec_time = event->exec_time >> 1;
    return 0;
}


void printEventStruct(struct Event_struct* event) {
	printf(" Event id %hhu, CPU %hhu, taskID %hu, jobId %u, when %llu, exec_time %llu, forced %hhu\n",
	event->event_id, 
    event->cpu,
	event->task_id,
	event->job_id,
	event->when,
    event->exec_time,
    event->was_forced);
};
