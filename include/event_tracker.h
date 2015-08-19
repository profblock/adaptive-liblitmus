#ifndef _EVENT_TRACKER_H_
#define _EVENT_TRACKER_H_

#define ENABLE_CMD  0x0
#define DISABLE_CMD	0x1

// event ID for schedcat job completions
#define ST_JOB_COMPLETION 506

//How much data in a binary completion record?
#define ST_JOB_COMPLETION_LEN 24 // bytes

struct core_file_struct {
	FILE* file;
	int coreID;
};

struct event_struct {
	unsigned char event_id;
    unsigned char cpu;
	unsigned short task_id;
	unsigned int job_id;
	unsigned long long when;
    unsigned long long exec_time;
    unsigned char was_forced;
};


void printEventStruct(struct event_struct* event);

int convert_data_stream_to_event_struct(void* data, size_t offset, struct event_struct* event);

struct core_file_struct* loadFiles(int numberOfCores);

int matchesFileStart(char* fileName, char* target);

int turn_events_on(FILE* file, int event_id);

int turn_events_off(FILE* file, int event_id);

int turn_non_blocking(FILE* file);

size_t read_trace_record(FILE* file, struct event_struct* data, size_t number_of_records_to_read); 


#endif 

