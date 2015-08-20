#ifndef _FEEDBACK_H_
#define _FEEDBACK_H_

struct execution_time_struct {
	long long cumulative_difference;
	long long current_difference;
	unsigned long long actual_execution;
	long long estimated_execution;	
};

long long calc_estimated(struct execution_time_struct * et, unsigned long long prev_actual, double p, double i);

#endif