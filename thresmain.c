#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include "mpi.h"
#include "ppmio.h"
#include "blurfilter.h"
//#include "thresfilter.h"
#include <math.h>

int main (int argc, char ** argv) {
  int xsize, ysize, colmax, root = 0, numrow, rowrest;
  uint sum, i, psum, nump, globalsum;
  pixel *src, *localsrc;
  //pixel src[MAX_PIXELS];
  // struct timespec stime, etime;

  int np, rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  double starttime;
  starttime = MPI_Wtime();
  
  int scounts[np], displs[np];

  //Create a pixel MPI struct
  MPI_Datatype mpi_pixel; /* MPI_Struct of three members */
  const int nitems = 3; /* Decalration of parts for the actual declaration of mpi_pixel below */
  int blocklengths[3] = {1,1,1};
  MPI_Datatype types[3] = {MPI_CHAR, MPI_CHAR, MPI_CHAR};
  MPI_Aint offsets[3];
  offsets[0] = offsetof(pixel, r);
  offsets[1] = offsetof(pixel, g);
  offsets[2] = offsetof(pixel, b);
  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pixel);
  MPI_Type_commit(&mpi_pixel);

  /* Take care of the arguments */
  if (rank == root)
    {
      if (argc != 3) {
	fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
	exit(1);
      }

      src = (pixel *) malloc(sizeof(pixel) * MAX_PIXELS);

      /* read file */
      if(read_ppm (argv[1], &xsize, &ysize, &colmax, (char *) src) != 0)
	exit(1);

      numrow = ceil(ysize/(double)np);
      rowrest = np*numrow % ysize;

      src = realloc(src, sizeof(pixel) * xsize*ysize);


      if (colmax > 255) {
	fprintf(stderr, "Too large maximum color-component value\n");
	exit(1);
      }

      //chop up
      for (i = 0; (int) i < np-1; ++i)
	{
	  scounts[i] = xsize*numrow;
	  displs[i] = scounts[i]*i;
	}
      {
	scounts[np-1] = xsize*(numrow - rowrest);
	displs[np-1] = scounts[0]*(np-1);
      }
      
    }
  
  MPI_Bcast(&xsize, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&ysize, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&numrow, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&rowrest, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&scounts, np, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&displs, np, MPI_INT, root, MPI_COMM_WORLD);

  localsrc = (pixel *) malloc(sizeof(pixel) * scounts[rank]);
  MPI_Scatterv(src, scounts, displs, mpi_pixel,
	       localsrc, scounts[0], mpi_pixel, root, MPI_COMM_WORLD);
  
  if (rank == 0)
    printf("Has read the image, calling filter\n");

  /* clock_gettime(CLOCK_REALTIME, &stime); */

  //thresfilter(xsize, ysize, src);

  if (rank == np-1)    {
      nump = xsize * (numrow - rowrest);
    }
  else    {
      nump = xsize * numrow;
    }

  // Do this for all processes, then send result and calculate global avg
  for(i = 0, sum = 0; i < nump; i++) {
    sum += (uint)localsrc[i].r + (uint)localsrc[i].g + (uint)localsrc[i].b;
  }

  sum /= nump;

  MPI_Reduce(&sum, &globalsum, 1, MPI_UNSIGNED,
	     MPI_SUM, root, MPI_COMM_WORLD);

  globalsum /= np;

  MPI_Bcast(&globalsum, 1, MPI_UNSIGNED, root, MPI_COMM_WORLD);

  // Do this again for all processes
  for(i = 0; i < nump; i++) {
    psum = (uint)localsrc[i].r + (uint)localsrc[i].g + (uint)localsrc[i].b;
    if(globalsum > psum) {
      localsrc[i].r = localsrc[i].g = localsrc[i].b = 0;
    }
    else {
      localsrc[i].r = localsrc[i].g = localsrc[i].b = 255;
    }
  }
  
  //clock_gettime(CLOCK_REALTIME, &etime);

  /* printf("Filtering took: %g secs for process %d\n", (etime.tv_sec  - stime.tv_sec) + */
  /* 	 1e-9*(etime.tv_nsec  - stime.tv_nsec), rank) ; */

  //gather local src:es to src.
  MPI_Gatherv(localsrc, scounts[rank], mpi_pixel, src, scounts, displs,
	      mpi_pixel, root, MPI_COMM_WORLD);

  if (rank == root)
    {
      /* write result */
      printf("Writing output file\n");
    
      if(write_ppm (argv[2], xsize, ysize, (char *)src) != 0)
	exit(1);
    }

  if (rank==0){
    double endtime = MPI_Wtime();
    printf("Elaplsed time: %g np: %d: num.pixels: %d\n",
	   endtime-starttime, np, xsize*ysize);
  }
  
  //free pointers

  if (rank == 0){
    free(src);
    src = NULL;
  }
  
  MPI_Finalize();

  return(0);
}
