#ifndef _WHISPER_H_
#define _WHISPER_H_






/***** NOISE SETUP ****/

typedef struct noiseStruct{
	long int noiseStartAfterLastInTics;
	long int noiseDurationInTics;
	double multiplictiveImpact;
	struct noiseStruct* next;
} noiseStruct;



/****** WHISPER STRUCTS AND METHODS *****/
/*Microphones are on the outside, speakers on the inside */
typedef struct micSpeakerStruct 
{
	long int micXPos;
	long int micYPos;

	//We maintain radians around unit circle rather than
	//raw X, Y position to keep the "one Truth"
	double initRadians;
	double curRadians;
	
	long int radius;	
	long int speedInUnitsPerTic;
	
	noiseStruct* noiseList;
	long int lastNoiseStart;
	long int lastNoiseEnd;
	//0 if currently noise, 1 if no noise
	int quiet;
	
	//Total Tics should not be used, and is just here for debugging purposes
	//i.e., validating that the curRadians (the "actual value") is legitimate
	long int totalTics;
} micSpeakerStruct;

/* Called to construct the whisper room. If not called, then everything takes on default 
 * values given above. 
 * Returns 1 if error, 0 otherwise
 */
 
/***** Construct and initialize  MicSpeaker Pairs *****/
// Returns 0 if fails
//NOTE: If you want noise, then this needs to be called after all the noises have been establied
micSpeakerStruct* constructSpeakerMicPair (int speakerNumber, int micNumber);

//return 0 if successful, return 1 if error
//NOTE: If you want noise, then this needs to be called after all the noises have been establied
int initSpeakerMicPair(micSpeakerStruct* ms, int speakerNumber, int micNumber);



/**** Occluding Points data *****/
typedef struct OccludingPointsStruct{
	long int x1;
	long int y1;
	long int x2;
	long int y2;
	int numberOfPoints; //Can be 0,1, or 2. 
} OccludingPointsStruct;

void occludingPoints(micSpeakerStruct* ms, OccludingPointsStruct* ops);

double getThetaBetweenTwoPoints(OccludingPointsStruct* ops);

/**************************************************/
/***************** KEY METHODS ********************/
/**************************************************/


/* Used to set up the WHISPER Environment. If not called, then default Values are used */
int initWhisperRoom(double alpha, double beta, int numSpeakers, int numMic, int occluding, long int occludingSize,
					long int side, long int radius, double speedInMetersPerSecond, long int unitsPerMeter, 
					long int ticksPerSecond); 

/* This will return a microphone speaker pair by number from 0 to NUMBER_OF_SPEAKERS*NUMBER_OF_MICROPHONES -1
 * Numbers outside of this range will fail. 
 * We work on one microphone at a time. 
 * This method is a convience so that way you put it easily into a for loop.
 * IF YOU WANT NOISE, this must be called after you set up the noise for the system.. 
 * returns 0 if fail
 */
micSpeakerStruct* constructSpeakerMicPairByNumber(int speakerPairNumber);

/* Updated the specfic micSpeaker pair by the number of Ticks value. number of ticks should 
 * probably be 1 unless there is a good reason
 */
void updatePosition(micSpeakerStruct* ms, long int numOfTicks);

/* Gets the number of operations to "process" the signal to noise ratio for the associated
 * microphone Speaker Pair. 
 */
int getNumberOfOperations(micSpeakerStruct* ms);

/* startAfterSeconds will start this noise this many seconds after the last noise ended
 * duration is how long this noise will last
 * will not work if durations are REALLY small. (We only pull one "noise" off at a time
 * So, if you pick durations that are basically 1 tic big, then we'll only see one of them at 
 * a time.
 */
int addNoise(double startAfterSeconds, double durationInSeconds, double factor);

double getMicSpeakerDistanceInMeters(micSpeakerStruct* ms);

// These are no constants Constant because they might be changeable by the initWhisper method.

/**** Conversion factors---> Abstract to Real *****/
//Conversion factor for the room, if this were 1. then each unit in the room would be 1 meter
//If this were a 1000, then 1 meter is 1000 units.
/*static*/ extern long int WHISPER_UNITS_IN_A_METER;

//Conversion factor for "tics" to updatePosition to Second
//If this were 1 then 1 tic equals 1 second
//If this were 100, then 100 tics equals 1 second. 
/*static*/ extern long int WHISPER_TICS_PER_SECOND;


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
/*static*/ extern double WHISPER_ALPHA; //Times (d+1), then squared
/*static*/ extern double WHISPER_BETA;  // Multiplied by above value



/***** Whisper Room Dimensions *****/
/*static*/ extern long int WHISPER_ROOM_SIDE;
/*static*/ extern long int WHISPER_SPEAKER_RADIUS;


/*set to 0 if there is no occluding object, 1 if there is */
/*static*/ extern int OCCLUDING_OBJECT;
/*static*/ extern int OCCLUDING_OBJECT_SIZE;

/***** EXPERIMENT SETUP ******/
// These are not Constant because they might be changeable by the initWhisper method.
/*static*/ extern int NUMBER_OF_SPEAKERS;
/*static*/ extern int NUMBER_OF_MICROPHONES;
/*static*/ extern double SPEAKER_SPEED_IN_M_PER_SEC;


/*static*/ extern noiseStruct* NOISE_LIST;
/*static*/ extern noiseStruct* END_LIST;

#endif