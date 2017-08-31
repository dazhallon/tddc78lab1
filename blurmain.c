#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h> // getpid
#include "ppmio.h"
#include "blurfilter.h" //gets also struct pixel
#include "gaussw.h"
#include "mpi.h"
#include <math.h> //ceil()
#include "subroutines.h"

int main (int argc, char ** argv) {
  int radius;
  int xsize, ysize, colmax, yfrom, yto, recvcount, sendcount;
  pixel *localsrc, *src, *sendbacksrc;

  const int root = 0;
  //pixel src[MAX_PIXELS];
  /* struct timespec stime, etime; */
#define MAX_RAD 1000

  double w[MAX_RAD];

  /* Take care of the arguments */

  MPI_Init(&argc, &argv);

 
  //Get rank of process
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int np;
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  double starttime;

  if (rank == 0){
    starttime = MPI_Wtime();
  }


  int scounts[np],  displs[np], filtercounts[np], sendbackdispls[np]; 

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

#ifdef DEBUG
  // DEBUGGING
  {
    int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d on %s ready for attach\n", getpid(), hostname);
    fflush(stdout);
    while (0 == i)
      sleep(1);
  }
#endif
    
  /* read file */
  if (rank == root)
    {
      if (argc != 4) {
	fprintf(stderr, "Usage: %s radius infile outfile\n", argv[0]);
	exit(1);
      }
      radius = atoi(argv[1]);
      if((radius > MAX_RAD) || (radius < 1)) {
	fprintf(stderr, "Radius (%d) must be greater than zero and less then %d\n", radius, MAX_RAD);
	exit(1);
      }
      
      src = (pixel*) malloc(MAX_PIXELS * sizeof(pixel));
      if(read_ppm (argv[2], &xsize, &ysize, &colmax, (char *) src) != 0)
	exit(1);
      if (colmax > 255) {
	fprintf(stderr, "Too large maximum color-component value\n");
	exit(1);
      }
     
      int len = xsize*ysize*sizeof(pixel);
      src = realloc(src, len);
      if (src == NULL)
	{
	  fprintf(stderr,"Pixel was not allocated correctly (== NULL)\n");
	}

      //Chop up the scr file into np equizised pices
      chopup(np, xsize, ysize, radius, scounts, displs,
	     filtercounts, sendbackdispls);
    
      get_gauss_weights(radius, w);
    }
  
  //MPI_Bcast(&chunkSize, 1, MPI_INT, root, MPI_COMM_WORLD);
   
  MPI_Bcast(&xsize, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&ysize, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&radius, 1, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&scounts, np, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&displs, np, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&filtercounts, np, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&sendbackdispls, np, MPI_INT, root, MPI_COMM_WORLD);
  MPI_Bcast(&w, sizeof(w)/sizeof(w[0]), MPI_DOUBLE, root, MPI_COMM_WORLD);

  recvcount = scounts[rank];
  localsrc = (pixel *) malloc(scounts[rank] * sizeof (pixel));
  
  /* if (rank == root) fprintf(stderr,"Process %d is sending data to all other processes\n", root); */
  //Might be sending to much data, but should not matter.
  if (MPI_Scatterv(src, scounts, displs, mpi_pixel, localsrc,
		   recvcount, mpi_pixel, root, MPI_COMM_WORLD) != MPI_SUCCESS) {
    /* fprintf(stderr, "FAILURE!\n"); */
  }/*  else { */
  /*   fprintf(stderr, "SUCCESS!\n"); */
  /* } */
   
  
  yfrom = 0;
  yto = scounts[rank]/xsize;
  
  /* fprintf(stderr, "Process %d is calling filter\n", rank); */
  /* clock_gettime(CLOCK_REALTIME, &stime); */
  blurfilter(xsize, yfrom, yto, localsrc, radius, w);   
  /* clock_gettime(CLOCK_REALTIME, &etime); */

  /* printf("Filtering took: %g secs for processs %d\n", (etime.tv_sec  - stime.tv_sec) + */
  /* 	 1e-9*(etime.tv_nsec  - stime.tv_nsec), rank) ; */

  //Return result from slaves to master

  if (rank == 0)    {
      sendbacksrc = localsrc;
    }
  else if (rank > 0)     {
      sendbacksrc = localsrc + radius*xsize;
    }

  MPI_Barrier(MPI_COMM_WORLD);
  sendcount = filtercounts[rank];
  MPI_Gatherv(sendbacksrc, sendcount, mpi_pixel,
	      src, filtercounts, sendbackdispls, mpi_pixel,
	      root, MPI_COMM_WORLD);

  /* write result */
  if (rank == root)
    {
      printf("Writing output file\n");
      
      if(write_ppm (argv[3], xsize, ysize, (char *)src) != 0)
	exit(1);
    }

  if (rank==0){
    double endtime = MPI_Wtime();
    printf("Elaplsed time: %g np: %d: num.pixels: %d\n",
	   endtime-starttime, np, xsize*ysize);
  }

  //Free the MPI_Data_types created.
  free(mpi_pixel);
  mpi_pixel = NULL;

  //Free pointers
  free(src);
  src = NULL;
  //  free(sendbacksrc);// can not free
  free(localsrc);
  localsrc = NULL;

  MPI_Finalize();
  
  return(0);
}
