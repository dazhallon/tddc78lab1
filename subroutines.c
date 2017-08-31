#include "subroutines.h"
#include <stdio.h>

int sumArray(const int* array, const int np)
{
  int sum = 0;
  for (int i = 0; i < np; ++i)
    {
      sum += array[i];
    }
  return sum;
}

void chopup(const int np, const int xsize, const int ysize, const int radius,
 	     int* scounts, int* displs,
	     int* filtercounts, int* sendbackdispls)
 {
   int chunkSize, numrow;
   numrow = ceil(ysize/(double)np);
   chunkSize = xsize*numrow;
   
   //Handle middle sections
   for (int i = 1; i < np-1; ++i)
     {
       scounts[i] = chunkSize + 2*radius*xsize;  
       displs[i] = chunkSize*i - radius*xsize;
       filtercounts[i] = chunkSize; //How many pixels should be filtered for proc. i
       sendbackdispls[i] = filtercounts[i]*i; //the displs for the pixels to be sent back.
     }

   //Handle special cases 
   {
     scounts[0] = chunkSize + radius*xsize;
     displs[0] = 0;

     filtercounts[0] = chunkSize;
     sendbackdispls[0] = 0;
     if (np > 1) // otherwise the code above will be overwritten
       {
	 filtercounts[np-1] = xsize*(numrow - ((numrow*np) % ysize));
 	 sendbackdispls[np-1] = xsize*ysize - filtercounts[np-1];

	 scounts[np-1] = filtercounts[np-1] + radius*xsize;
	 displs[np-1] = chunkSize*(np-1) - radius*xsize;
       }
   }
 }
