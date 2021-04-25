#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <fstream>
#include <iostream>


//size of superquad
#define XMIN        -1.
#define XMAX        1.
#define YMIN        -1.
#define YMAX        1.
//number of nodes
#ifndef NUMNODES
#define NUMNODES    100       
#endif
//number of threads 
#ifndef NUMT
#define NUMT		1
#endif
//superquad exponent 
#define N           0.70

// function declarations
float Height(int, int);



// calculate height of each node point from iu,iv = 0 to NUMNODES-1
float Height( int iu, int iv )
{
	float x = -1.  +  2.*(float)iu /(float)(NUMNODES-1);	// -1. to +1.
	float y = -1.  +  2.*(float)iv /(float)(NUMNODES-1);	// -1. to +1.

	float xn = pow( fabs(x), (double)N );
	float yn = pow( fabs(y), (double)N );
	float r = 1. - xn - yn;
	if( r <= 0. )
	        return 0.;
	float height = pow( r, 1./(float)N );
	return height;
}


int main( int argc, char *argv[ ] )
{
    #ifndef _OPENMP
        fprintf( stderr, "No OpenMP support!\n" );
        return 1;
    #endif

	omp_set_num_threads( NUMT );	// set the number of threads to use in parallelizing the for-loop

	// the area of a single full-sized tile: 
	float fullTileArea = (((XMAX-XMIN) / (float)(NUMNODES-1)) * ((YMAX-YMIN) / (float)(NUMNODES-1)));
    float volume = 0.;


    double time0 = omp_get_wtime( ); //start timer 

	// sum up the weighted heights into the variable "volume"
	// using an OpenMP for loop and a reduction:
    #pragma omp parallel for default(none), shared(stdout), shared(fullTileArea), reduction(+:volume)
    for( int i = 0; i < (NUMNODES*NUMNODES); i++ )
    {
        int iu = i % NUMNODES; // node # along x axis
        int iv = i / NUMNODES; // node # along y axis
        float z = Height( iu, iv ); //height at node 
        float nodeVolume = 0.; // volume at current node

        // fprintf(stdout, "iu: %d, iv: %d", iu, iv);

        if ((iu == 0 || iu == NUMNODES-1) && (iv == 0 || iv == NUMNODES-1)) { // if (iu, iv) is a corner node: 1/4 full area
            nodeVolume = 0.25 * fullTileArea * z;
            // fprintf(stdout, ", quarter\n");
        }
        else if (iu == 0 || iu == NUMNODES-1 || iv == 0 || iv == NUMNODES-1) { // if (iu, iv) is an edge node: 1/2 full area
            nodeVolume = 0.5 * fullTileArea * z;
            // fprintf(stdout, ", half\n");
        }
        else {  // if (iu, iv) is a middle node: full area
            nodeVolume = fullTileArea * z;
            // fprintf(stdout, ", full\n");
        }
        volume += nodeVolume;
    }

    volume = volume * 2.; // double volume to account for bottom half of superquad volume

    double time1 = omp_get_wtime( ); //end timer 
    double megaHeightsPerSec = (NUMNODES * NUMNODES) / ( time1 - time0 ) / 1000000.;

    // open data CSV file
	std::ofstream dataFile;
	dataFile.open("superQuadData.csv", std::ios_base::app);

    fprintf(stdout, "%2d threads : %8d nodes ; volume = %6.2f ; megaHeights/sec = %6.2lf\n",
		NUMT, NUMNODES, volume, megaHeightsPerSec);

    //write data to csv file
	dataFile << NUMT << "," << NUMNODES << "," << megaHeightsPerSec << "," << volume << "\n";
}