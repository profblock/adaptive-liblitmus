#include "whisper.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>



/**** Conversion factors---> Abstract to Real *****/
//Conversion factor for the room, if this were 1. then each unit in the room would be 1 meter
//If this were a 1000, then 1 meter is 1000 units.
long int WHISPER_UNITS_IN_A_METER;

//Conversion factor for "tics" to updatePosition to Second
//If this were 1 then 1 tic equals 1 second
//If this were 100, then 100 tics equals 1 second. 
long int WHISPER_TICS_PER_SECOND;


/***** Number of Calculation works *******/
// The number of operations necessary is based on the signal-to-noise-ratio that decays
// quadratically based on the distance between the microphone speaker pair. 
// 				newSignalToNoise = beta (alpha * (distance+1))^2
// The problem we run into is that this is a simulated environment so the alpha and beta values
// are not known. 
// In this work, it doesn't matter too much what the actual beta and alpha values are
// because we are not trying to model Whisper exactly, but rather we are using a 
// "whisper-like" workload where the amount of work increases quadratically based on distance.
// So, it stand to reason, that there is probably some work (or perhaps some implementation of Whisper)
// that would have whatever alpha and beta values we choose. 

//For reference, we will measure "distance" above in Meters. So, we'll need to convert "units" to "meter"
double WHISPER_ALPHA; //Times (d+1), then squared
double WHISPER_BETA;  // Multiplied by above value



/***** Whisper Room Dimensions *****/
long int WHISPER_ROOM_SIDE;
long int WHISPER_SPEAKER_RADIUS;


/*set to 0 if there is no occluding object, 1 if there is */
int OCCLUDING_OBJECT;
int OCCLUDING_OBJECT_SIZE;

/***** EXPERIMENT SETUP ******/
// These are not Constant because they might be changeable by the initWhisper method.
int NUMBER_OF_SPEAKERS;
int NUMBER_OF_MICROPHONES;
double SPEAKER_SPEED_IN_M_PER_SEC;


noiseStruct* NOISE_LIST;
noiseStruct* END_LIST;

/****** "Private" Method Declaration, Definitions Below ********/
double getCurRadians(micSpeakerStruct* ms);
long int getXSpeakerPos(micSpeakerStruct* ms);
long int getYSpeakerPos(micSpeakerStruct* ms);
void defaultValues();


	

/****** "Public" Methods********/


int initWhisperRoom(double alpha, double beta, int numSpeakers, int numMic, int occluding, long int occludingSize,
					long int side, long int radius, double speedInMetersPerSecond, long int unitsPerMeter, 
					long int ticksPerSecond){
	defaultValues();
	if(numMic!=4){
		printf("Error: Only supports 4 microphones currently\n");
		return 1;
	}
	if (OCCLUDING_OBJECT_SIZE> radius){
		printf("ERROR: Occluding object must be less then the radius of the microphones\n");
		return 1;
	}
	SPEAKER_SPEED_IN_M_PER_SEC = speedInMetersPerSecond;
	
	NUMBER_OF_MICROPHONES = numMic;

	WHISPER_ALPHA = alpha; 
	WHISPER_BETA = beta;  
	NUMBER_OF_SPEAKERS = numSpeakers;
	
	OCCLUDING_OBJECT = occluding;
	if (!( (OCCLUDING_OBJECT==0) || (OCCLUDING_OBJECT ==1) )){
		printf("Warning: occluding should be either 0 or 1, setting to 1 and continuing");
	}
	WHISPER_ROOM_SIDE = side;
	WHISPER_SPEAKER_RADIUS = radius;
	if (side< (2*radius)){
		printf("Warning: The radius * 2 cannot be bigger than side. Setting side =2*radius and continuing");
		WHISPER_ROOM_SIDE = 2*WHISPER_SPEAKER_RADIUS;
	}
	
	OCCLUDING_OBJECT_SIZE = occludingSize;
	WHISPER_UNITS_IN_A_METER = unitsPerMeter;
	WHISPER_TICS_PER_SECOND = ticksPerSecond;
	return 0;
}


micSpeakerStruct* constructSpeakerMicPair (int speakerNumber, int micNumber){
	micSpeakerStruct* rtnStruct = (micSpeakerStruct*) malloc(sizeof(micSpeakerStruct));
	int returnVal;

	if(rtnStruct!=0){
		returnVal = initSpeakerMicPair(rtnStruct,speakerNumber,micNumber);
		if (returnVal==0){
			return rtnStruct;
		} else {
			printf("Error: Couldn't initialize speakerMicPair\n");
			return 0;
		}
	}
	printf("Error: Couldn't construct speakerMicPair\n");
	return 0;
}

micSpeakerStruct* constructSpeakerMicPairByNumber(int speakerPairNumber){
	int numberOfPairs = NUMBER_OF_SPEAKERS*NUMBER_OF_MICROPHONES;
	int micNumber;
	int speakerNumber;

	if((speakerPairNumber<0) || (speakerPairNumber>= numberOfPairs)){
		printf("Error: Number out of range\n");
		return 0;
	}
	
	micNumber = speakerPairNumber/NUMBER_OF_SPEAKERS;
	speakerNumber = speakerPairNumber % NUMBER_OF_SPEAKERS;
	printf("micNumber: %d, SpeakerNumber, %d\n", micNumber,speakerNumber);
	return constructSpeakerMicPair(speakerNumber,micNumber);
}


					
int initSpeakerMicPair(micSpeakerStruct* ms, int speakerNumber, int micNumber){
	double radianOnCircle;
	double speedInUnitsPerSecond;
	long int halfRoomSize;
	//double distance;
	if (NUMBER_OF_MICROPHONES !=4) {
		printf("ERROR: This only works for 4 Microphones right now");
		return 1;
	} 
	
	if (NOISE_LIST==0){
		printf("NOTE: No noise list has been established. If you wish to use noise, establish that first in your code\n");
	} 
	ms->noiseList = NOISE_LIST;
	ms->lastNoiseStart = -1; //It starts at -1 So there is no noise started at 0. 
	ms->lastNoiseEnd = 0; //It ends at 0 because thats way all future noise computations can be handled correctly
	ms->quiet=1; //Starts off quiet
		
	/* Establish the initial and current speaker Position */
	// Note M_PI is defined in math.h
	radianOnCircle = 2*M_PI*((double)speakerNumber)/NUMBER_OF_SPEAKERS;
	
	ms->initRadians = radianOnCircle;
	ms->curRadians = ms->initRadians;
	ms->radius = WHISPER_SPEAKER_RADIUS;
	
	speedInUnitsPerSecond= SPEAKER_SPEED_IN_M_PER_SEC* WHISPER_UNITS_IN_A_METER;
	ms->speedInUnitsPerTic = (long int) (speedInUnitsPerSecond/WHISPER_TICS_PER_SECOND);
	ms->totalTics = 0;


	/* Establish the initial Microphone Position */	
	//I know this is repetitive right now, but it won't be in the future 
	halfRoomSize = WHISPER_ROOM_SIDE/2;
	if (NUMBER_OF_MICROPHONES==4){
		switch (micNumber)
		{
			case 0: //Upper Right
				ms->micXPos = halfRoomSize;
				ms->micYPos = halfRoomSize;
				break;
			case 1: //Upper left
				ms->micXPos = -halfRoomSize;
				ms->micYPos = halfRoomSize;
				break;
			case 2: //lower left
				ms->micXPos = -halfRoomSize;
				ms->micYPos = -halfRoomSize;
				break;
			case 3: //lower right
				ms->micXPos = halfRoomSize;
				ms->micYPos = -halfRoomSize;
				break;
			default:
				printf("Error: number of microphones is out of range\n");
				return 1;
		}
	}
	//distance = getMicSpeakerDistanceInMeters(ms);
	
	/****** Debugging Messages *******
	 *printf("the Speed in Units Per Tic is %ld\n", ms->speedInUnitsPerTic);
	 *printf("The value is %ld, %ld\n", getXSpeakerPos(ms), getYSpeakerPos(ms));
	 *printf("The Mic (X,Y) is (%ld, %ld)\n", ms->micXPos, ms->micYPos);
	 *printf("The distance is %f\n", distance);
	 */
	return 0;
}


void updatePosition(micSpeakerStruct* ms, long int numOfTicks){

	long int unitsTraveledAroundArc;
	double fractionOfCircleTravled;
	double radiansTravled;
	ms->totalTics+=numOfTicks;
	
	
	unitsTraveledAroundArc = numOfTicks* ms->speedInUnitsPerTic;
	fractionOfCircleTravled = ((double)unitsTraveledAroundArc)/(2* M_PI* ms->radius);
	radiansTravled = 2*M_PI*fractionOfCircleTravled;
	ms->curRadians+=radiansTravled;	
	//printf("\t\t\tNumber of Ticks %ld, total:%ld, unitsTra:%ld, frac:%f\n", numOfTicks,ms->totalTics,unitsTraveledAroundArc,fractionOfCircleTravled) ;
	
// 	Testing code */
// 	 unitsTraveledAroundArc = ms->totalTics* ms->speedInUnitsPerTic;
// 	 fractionOfCircleTravled = ((double)unitsTraveledAroundArc)/(2* M_PI* ms->radius);
// 	 radiansTravled = 2*M_PI*fractionOfCircleTravled +ms->initRadians;

	if( (ms->noiseList!=0) && (ms->quiet==1) 
		&& (ms->totalTics >= (ms->lastNoiseEnd + ms->noiseList->noiseStartAfterLastInTics)) )
	{
		//printf("--------Starting noise at %ld\n", ms->totalTics);
		ms->quiet = 0;
		ms->lastNoiseStart = ms->lastNoiseEnd + ms->noiseList->noiseStartAfterLastInTics;
	}
	if( (ms->noiseList!=0) && (ms->quiet==0) 
		&& (ms->totalTics >= (ms->lastNoiseStart + ms->noiseList->noiseDurationInTics)) )
	{
		//printf("+++++++++ending noise at %ld\n", ms->totalTics);
		ms->quiet = 1;
		ms->lastNoiseEnd = ms->lastNoiseStart + ms->noiseList->noiseDurationInTics;
		ms->noiseList= ms->noiseList->next;
	}
	/**** Remove this later, for testing Only ****/
	/*	ms->totalTics+=numOfTicks;
	 * unitsTraveledAroundArc = ms->totalTics* ms->speedInUnitsPerTic;
	 * fractionOfCircleTravled = ((double)unitsTraveledAroundArc)/(2* M_PI* ms->radius);
	 * radiansTravled = 2*M_PI*fractionOfCircleTravled +ms->initRadians;
	 * printf("curRadians %f\ntotRadians %f\n",ms->curRadians, radiansTravled);
	 */
}
	

int getNumberOfOperations(micSpeakerStruct* ms){
	double radians;
	double additionalDistance;
	double totalComputations;
	double distanceFactor = getMicSpeakerDistanceInMeters(ms);
	//printf("Base Distance :%f, ", distanceFactor);
	
	//If there is an occluding object, then we need to add that distance
	if(OCCLUDING_OBJECT==1){
		OccludingPointsStruct* ops = (OccludingPointsStruct*)malloc(sizeof(OccludingPointsStruct));
		occludingPoints(ms, ops);
		if(ops->numberOfPoints==2){
			radians = getThetaBetweenTwoPoints(ops);
			
			//The distance around the circle needs to be converted from Units into Meters
			additionalDistance = OCCLUDING_OBJECT_SIZE*radians/WHISPER_UNITS_IN_A_METER;
			distanceFactor+=additionalDistance;
			
			//printf("Total Distance :%f, ", distanceFactor);
			//printf("Additional Distance %f, ", additionalDistance);
			
		} else if (ops->numberOfPoints==1){
			//Nothing happens because there is occlusion but no additional distance
			//I put this in here incase there is something to be done here in the future
		}
		//Nothing happens because there is no occlusion!
	}
	
	distanceFactor*=WHISPER_ALPHA;
	totalComputations = WHISPER_BETA * pow(distanceFactor,2);
	
	//If there is noise, the multiply the amount of computations by a factor.
	if( (ms->noiseList!=0) && (ms->quiet ==0) ) {
		totalComputations*=ms->noiseList->multiplictiveImpact;
	}
	
	return (int) totalComputations;
}

int addNoise(double startAfterSeconds, double durationInSeconds,double factor){
	noiseStruct* newNoise = (noiseStruct*)malloc(sizeof(noiseStruct));
	if(newNoise==0){
		printf("Error: Could not create noise element");
		return 1;
	}
	
	newNoise->noiseStartAfterLastInTics = (long int)(startAfterSeconds*WHISPER_TICS_PER_SECOND);
	newNoise->noiseDurationInTics = (long int)(durationInSeconds*WHISPER_TICS_PER_SECOND);
	newNoise->multiplictiveImpact=factor;
	newNoise->next = 0;
	
	if(NOISE_LIST==0){
		NOISE_LIST = newNoise;
		END_LIST = newNoise;
	} else {
		END_LIST->next = newNoise;
		END_LIST = newNoise;
	}
	return 0;
} 


//Value is returned in the ops datastructure. Must be created beforehand
void occludingPoints(micSpeakerStruct* ms, OccludingPointsStruct* ops){
	double sqrTerm;
	long int speakerX = getXSpeakerPos(ms);
	long int speakerY = getYSpeakerPos(ms);
	long int micX = ms->micXPos;
	long int micY = ms->micYPos;
	long int r = OCCLUDING_OBJECT_SIZE;
	
	double micSpeakXDiff = micX-speakerX;
	double micSpeakYDiff = micY-speakerY;
	double micSpeakerDistance = sqrt(pow(micSpeakXDiff,2) + pow(micSpeakYDiff,2));
	
	double m;
	double c;
	double alpha;
	double beta;
	double gamma;
	
	double xTerm;
	double yTerm;
	
	double xTerm1;
	double yTerm1;
	double xTerm2;
	double yTerm2;
	
	double micTerm1XDiff;
	double micTerm1YDiff;
	double micTerm1Distance;


	
	//printf("Speaker: (%ld,%ld), Mic:(%ld, %ld), ",speakerX, speakerY, micX, micY);
	if(speakerX==micX){
		//NOTE: I haven't tested this case. But it's simple and shouldn't arrise.  
		//Special case where on the same X axis
		printf("SpecialX: UNTESTED!\n");
		sqrTerm = pow(r,2) - pow(micX,2);
		if (sqrTerm<0) {
			//No Occlusion, nothing to see here.
			ops->numberOfPoints = 0; 
			return;
		} else if (sqrTerm==0){
			ops->numberOfPoints = 1;
			ops->x1 = micX;
			ops->y1 = 0; //Must be on the x-Axis. So It needs to be zero!
			ops->x2 = micX;
			ops->y2 = 0;
			return;
		} else {
			ops->numberOfPoints = 2;
			ops->x1 = (long int)micX;
			ops->y1 = (long int)sqrt(sqrTerm);
			ops->x2 = (long int)micX;
			ops->y2 = (long int)(-sqrt(sqrTerm));
			return;
		}			
	} else if (speakerY==micY){
		//special case where on same Y axis
		printf("SpecialY: UNTESTED\n");
		sqrTerm = pow(r,2) - pow(micY,2);
		if (sqrTerm<0) {
			//No Occlusion, nothing to see here.
			ops->numberOfPoints = 0; 
			return;
		} else if (sqrTerm==0){
			ops->numberOfPoints = 1;
			ops->x1 = 0;//Must be on the y-Axis. So It needs to be zero!
			ops->y1 = micY; 
			ops->x2 = 0;
			ops->y2 = micY;
			return;
		} else {
			ops->numberOfPoints = 2;
			ops->x1 = (long int)sqrt(sqrTerm);
			ops->y1 = (long int)micY;
			ops->x2 = (long int)(-sqrt(sqrTerm));
			ops->y2 = (long int)micY;
			return;
		}	
	} else {
		m = ((double)(speakerY-micY))/ (speakerX-micX);
		c = micY - m * micX;
		alpha = pow(m,2) + 1;
		beta = 2*m*c;
		gamma = pow(c,2)-pow(r,2);
		sqrTerm = pow(beta,2) - 4*alpha*gamma;
		if (sqrTerm<0){
			//No Occlusion, nothing to see here.
			ops->numberOfPoints = 0; 
			return;
		} else if (sqrTerm==0){
			
			xTerm = (-beta)/ (2*alpha);
			yTerm = m*xTerm +c;
			ops->numberOfPoints = 1;
			//For saftey reason, we store the one point in both (x1,y1) and (x2,y2)
			ops->x1 = (long int)xTerm;
			ops->y1 = (long int)yTerm;
			ops->x2 = (long int)xTerm;
			ops->y2 = (long int)yTerm;
			ops->numberOfPoints = 1;
			return;

		} else {
			xTerm1 = (-beta+ sqrt(sqrTerm))/ (2*alpha);
			yTerm1 = m*xTerm1 +c;
			xTerm2 = (-beta- sqrt(sqrTerm))/ (2*alpha);
			yTerm2 = m*xTerm2 +c;
			
			//The formula above calculates intersection based on an infinte line
			//However, we only one a line segment that begins at the microphone 
			//and ends at the speaker. 
			//So, we do not including the occlusion if that line segment is longer than
			//the occluding line segment. 
			//It is sufficient to check distance to one of the occlusions because
			//the speaker cannot be inside of the occlusion. 
			
			micTerm1XDiff = micX-xTerm1;
			micTerm1YDiff = micY-yTerm1;
			micTerm1Distance = sqrt(pow(micTerm1XDiff,2) + pow(micTerm1YDiff,2));
			if (micSpeakerDistance >micTerm1Distance) {
				ops->x1 = (long int)xTerm1;
				ops->y1 = (long int)yTerm1;
				ops->x2 = (long int)xTerm2;
				ops->y2 = (long int)yTerm2;
				ops->numberOfPoints = 2;
				return;
			} else {
				//Intersection on INFINITE LINE but not on this line segment
				ops->numberOfPoints = 0; 
				return;
			}
		}
	
	}
}
		
/****** "Private" Method Definition********/
//Incase I want to change how I calculate the current Radians
double getCurRadians(micSpeakerStruct* ms){
	return ms->curRadians;
}

//"private" method
long int getXSpeakerPos(micSpeakerStruct* ms){
	double xPos = cos(getCurRadians(ms)) * ms->radius;
	return (long int)xPos;
}

//"private" method
long int getYSpeakerPos(micSpeakerStruct* ms){
	double yPos = sin(getCurRadians(ms)) * ms->radius;
	return (long  int)yPos;
}

//"private" method for calculating the distance between the x and y
double getMicSpeakerDistanceInMeters(micSpeakerStruct* ms){
	long int speakerX = getXSpeakerPos(ms);
	long int speakerY = getYSpeakerPos(ms);
	long int micX = ms->micXPos;
	long int micY = ms->micYPos;
	
	long int xDis = speakerX-micX;
	long int yDis = speakerY-micY;
	
	double distInUnits = sqrt(pow(xDis,2) + pow(yDis,2));
	double distInMeters = distInUnits/WHISPER_UNITS_IN_A_METER;
	
	//printf("(%ld, %ld),  Distance is Meters %f", speakerX, speakerY, distInMeters);
	return distInMeters;
}


double getThetaBetweenTwoPoints(OccludingPointsStruct* ops){
	if (ops->numberOfPoints==2){
		double sideA = sqrt( pow(ops->x1-ops->x2,2) + pow(ops->y1-ops->y2,2));
		double sideB = sqrt( pow(ops->x2,2) + pow(ops->y2,2));
		double sideC = sqrt( pow(ops->x1,2) + pow(ops->y1,2));
		double term = (pow(sideB,2) + pow(sideC,2) - pow(sideA,2))/(2*sideB*sideC);
		double angle = acos(term);
		//printf("Angle %f, ", angle);
		return angle;
	} else {
		return 0;
	}
}

void defaultValues()
{
	/**** Conversion factors---> Abstract to Real *****/
	//Conversion factor for the room, if this were 1. then each unit in the room would be 1 meter
	//If this were a 1000, then 1 meter is 1000 units.
	WHISPER_UNITS_IN_A_METER = 100000;

	//Conversion factor for "tics" to updatePosition to Second
	//If this were 1 then 1 tic equals 1 second
	//If this were 100, then 100 tics equals 1 second. 
	WHISPER_TICS_PER_SECOND = 100;


	/***** Number of Calculation works *******/
	// The number of operations necessary is based on the signal-to-noise-ratio that decays
	// quadratically based on the distance between the microphone speaker pair. 
	// 				newSignalToNoise = beta (alpha * (distance+1))^2
	// The problem we run into is that this is a simulated environment so the alpha and beta values
	// are not known. 
	// In this work, it doesn't matter too much what the actual beta and alpha values are
	// because we are not trying to model Whisper exactly, but rather we are using a 
	// "whisper-like" workload where the amount of work increases quadratically based on distance.
	// So, it stand to reason, that there is probably some work (or perhaps some implementation of Whisper)
	// that would have whatever alpha and beta values we choose. 

	//For reference, we will measure "distance" above in Meters. So, we'll need to convert "units" to "meter"
	WHISPER_ALPHA = 4; //Times (d+1), then squared
	WHISPER_BETA = 1.2;  // Multiplied by above value



	/***** Whisper Room Dimensions *****/
	WHISPER_ROOM_SIDE = 2000000;
	WHISPER_SPEAKER_RADIUS = 800000;


	/*set to 0 if there is no occluding object, 1 if there is */
	OCCLUDING_OBJECT = 1;
	OCCLUDING_OBJECT_SIZE = 250;

	/***** EXPERIMENT SETUP ******/
	// These are not Constant because they might be changeable by the initWhisper method.
	NUMBER_OF_SPEAKERS = 8;
	NUMBER_OF_MICROPHONES = 4;
	SPEAKER_SPEED_IN_M_PER_SEC = 0.02;


	NOISE_LIST= 0;
	END_LIST = 0;
}

