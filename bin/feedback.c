//Feedback methods

#include <stdio.h>
#include <stdlib.h>
#include "feedback.h"

/* 	p and i values are the weights that should be given to the proportional and
 *	integrative components for calculating a new estimated execution time 
 * 	Retuns the estimated and also updated the estimated in the struct */
 
 
long long calc_estimated(struct execution_time_struct * et, unsigned long long prev_actual, double p, double i){
	/* update the cumulative estimated execution difference */	
	et->cumulative_difference += et->current_difference;
	et->actual_execution = prev_actual;
	
	et->current_difference = et->actual_execution - et->estimated_execution;
	
	/* Update the difference between the estimated and actual execution time*/
 	
 
	et->estimated_execution = (long long )(p * et->current_difference + i * et->cumulative_difference);		
	return et->estimated_execution;
}