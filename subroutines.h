/*Here are som e function that are used in the lab and are moved here
to avoid that the code looks crowded.
*/

#include <math.h>

int sumArray(const int* array, const int np);
  
void chopup(const int np, const int xsize, const int ysize, const int radius, 
	    int scounts[np], int displs[np],
	    int filtercounts[np], int sendbackdispls[np]);
