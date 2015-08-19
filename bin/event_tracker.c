// Event Tracker implementation file
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 


#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>

#include <sys/fcntl.h>
#include "event_tracker.h"


// *********************************
// METHODS FOR LOADING THE TRACING INFORMATION 
// **********************************
struct core_file_struct* loadFiles(int numberOfCores){
	DIR* fileDirectory; 
	//OPen the directory
	struct dirent* fileStruct;
	char* targetDirectory = "/dev/litmus/";
	char* targetFileName = "sched_trace\0";
	int fileNumber =-1;
	struct core_file_struct*  array_of_files = (struct core_file_struct*) malloc(sizeof(struct core_file_struct)*numberOfCores);
	char fileName[1024]; //We assume that filenames are at most 1024 charcters

	if (array_of_files == 0) {
		fprintf(stderr, "Error :Out of memmory\n");
		return NULL;
	}

	fileDirectory = opendir(targetDirectory);
	if (NULL == fileDirectory) {
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

			snprintf(fileName, sizeof(fileName), "%s%s",targetDirectory,fileStruct->d_name);

			array_of_files[fileNumber].coreID = fileNumber;
			array_of_files[fileNumber].file = fopen(fileName, "rb");
			if (array_of_files[fileNumber].file == NULL)
			{
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
			fprintf(stderr, "Error : Too many files - %s\n", strerror(errno));
			free(array_of_files);
			return NULL;
		}
	}
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
		fprintf(stderr, "Error : file not found - %s\n", strerror(errno));
		return -1;
	}	

	return ioctl(fd,DISABLE_CMD, event_id);
}

size_t read_trace_record(FILE* file, struct event_struct* data, size_t number_of_records_to_read) {
	size_t recordsRead; 
	size_t i;
	size_t offset=0;
	struct event_struct* eventStructTempPointer;
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
		eventStructTempPointer +=sizeof(struct event_struct);
	}
	free(tempStore);
	return records_actually_read;
	
}

int convert_data_stream_to_event_struct(void* data, size_t offset, struct event_struct* event){
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


void printEventStruct(struct event_struct* event) {
	printf(" Event id %hhu, CPU %hhu, taskID %hu, jobId %u, when %llu, exec_time %llu, forced %hhu\n",
	event->event_id, 
    event->cpu,
	event->task_id,
	event->job_id,
	event->when,
    event->exec_time,
    event->was_forced);
};
