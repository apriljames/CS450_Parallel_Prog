
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <fstream>
#include <iostream>

// state of the system
int	        NowYear;		// 2021 - 2026
int	        NowMonth;		// 0 - 11

float	        NowPrecip;		// inches of rain per month
float	        NowTemp;		// temperature this month
float	        NowHeight;		// grain height in inches
int	        NowNumDeer;		// number of deer in the current population
int             NowTourists;            // number of tourists at the park at a given day. the more tourists, the more they feed the deer 

// important parameters
const float GRAIN_GROWS_PER_MONTH =		9.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;
// Units of grain growth are inches.
// Units of temperature are degrees Fahrenheit (°F).
// Units of precipitation are inches.
// Time step: one month. One year: 12 months, 30 days per month. January 1 = first day of winter.

unsigned int seed = 0; // for randomization 

//helper function to square a float 
float SQR( float x ) { 
        return x*x;
}

// randomizer helper functions
float Ranf( unsigned int *seedp,  float low, float high ) {
        float r = (float) rand_r( seedp );              // 0 - RAND_MAX
        return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}

int Ranf( unsigned int *seedp, int ilow, int ihigh ) {
        float low = (float)ilow;
        float high = (float)ihigh + 0.9999f;
        return (int)(  Ranf(seedp, low,high) );
}

// "Watcher" thread: prints global state variables and handles the change of environment variables 
void Watcher() {
        while( NowYear < 2027 ) {
        // Do nothing in Watcher thread here

        // DoneComputing barrier:
        #pragma omp barrier
        // Do nothing in Watcher thread here

        // DoneAssigning barrier:
        #pragma omp barrier
        // print global state variables 
        fprintf( stdout, "Year: %d, Month: %d, Precip: %.3f, Temp: %.3f, Height: %.3f, Deer: %d, Tourists: %d\n" , NowYear, NowMonth, NowPrecip, NowTemp, NowHeight, NowNumDeer, NowTourists);
        
        // open data CSV file
	std::ofstream dataFile;
	dataFile.open("simulationData.csv", std::ios_base::app);

        //write data to csv file
	dataFile << NowYear << "," << NowMonth << "," << NowPrecip << "," << NowTemp << "," << NowHeight << "," <<  NowNumDeer << "," << NowTourists << "\n";
        // increment month
        NowMonth++;
        // increment year if applicable 
        if (NowMonth == 11) {
                NowMonth = 0;
                NowYear ++;
        }
        // compute new temperature and precipitation
        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. ) {
                NowPrecip = 0.;
        }

        // DonePrinting barrier:
        #pragma omp barrier
        }   
}

// "Deer" thread: calculates the next number of deer based on current state. 
void Deer() {
        while( NowYear < 2027 ) {
                int NextNumDeer = NowNumDeer;
                int carryingCap = (int)(NowHeight);
                // regulate the number of deer based on grain- height of grain = deer Carrying Capacity
                if (NowNumDeer > carryingCap) { //if number of deer is higher than grain height, 1 deer disappears 
                        NextNumDeer --;
                }
                else if (NowNumDeer < carryingCap) { //if number of deer is less than grain height, 1 deer grows 
                        NextNumDeer ++;
                }

                // regulate deer based on tourists who feed deer. The more tourists, the more they feed the deer 
                // more food = more deer. Less tourists = less deer 
                int fluxDeer;
                if (NowTourists >= 18) {
                        fluxDeer = (NowTourists - 18) / 5;
                        NextNumDeer += fluxDeer;
                }
                else if (NowTourists < 18) {
                        fluxDeer = (NowTourists) / 5;
                        NextNumDeer -= (3 - fluxDeer);
                }


                if( NextNumDeer < 0 ) { // clamp number of deer to zero 
                        NextNumDeer = 0;
                }

                // DoneComputing barrier:
                #pragma omp barrier
                // copy Next state into the Now variables 
                NowNumDeer = NextNumDeer;

                // DoneAssigning barrier:
                #pragma omp barrier
                // do nothing here- wait for Watcher to finish updating global state variables 

                // DonePrinting barrier:
                #pragma omp barrier
        }
}

// "Grain" thread: calculates the height of grain based on current state.
void Grain() {
        while( NowYear < 2027 ) {
                // determine weather conditions 
                float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
                float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
                // determine grain height based on temp and precipitation 
                float nextHeight = NowHeight;
                nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
                nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
                if( nextHeight < 0. ) { // clamp nextHeight against zero
                        nextHeight = 0.;
                }
                // DoneComputing barrier:
                #pragma omp barrier
                // copy next state variables into now state 
                NowHeight = nextHeight;

                // DoneAssigning barrier:
                #pragma omp barrier
                // do nothing here. wait for watcher thread to finish bookkeeping 

                // DonePrinting barrier:
                #pragma omp barrier
        }
}

// "Tourists" thread: calculates the influx of tourists based on the season. They feed the deer delicious deer food. 
void Tourists() {
        while( NowYear < 2027 ) {
                // compute a temporary next-value for this quantity
                // based on the current state of the simulation:
                int AvgTourists = 20; 

                // number of tourists peaks in the summer and falls in the winter. 
                float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );
                int tempTourists =  - 10 * cos( ang - (3.1415/4.)) + AvgTourists;
                if( tempTourists < 0. ) { // clamp number of tourists to zero (shouldn't reach this point )
                        tempTourists = 0.;
                }
                if (NowYear == 2024) { // the number 4 is unlucky... no one wants to travel right now! 
                        tempTourists -= 5;
                }
                
                // DoneComputing barrier:
                #pragma omp barrier
                // assign temp tourists to NowTourists
                NowTourists = tempTourists;

                // DoneAssigning barrier:
                #pragma omp barrier
                // . . .

                // DonePrinting barrier:
                #pragma omp barrier
                // . . .
        }
}

int main( int argc, char *argv[ ] ) {
        // starting date and time:
        NowMonth =    0;
        NowYear  = 2021;

        // starting state (feel free to change this if you want):
        NowNumDeer = 20;
        NowHeight =  5.;

        omp_set_num_threads( 4 );	// four threads, corresponding to four sections
        #pragma omp parallel sections
        {
                #pragma omp section
                {
                        Deer( );
                }

                #pragma omp section
                {
                        Grain( );
                }

                #pragma omp section
                {
                        Watcher( );
                }

                #pragma omp section
                {
                        Tourists( );
                }
        }       // implied barrier -- all functions must return in order
                // to allow any of them to get past here


        return 0;
}