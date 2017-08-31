LFLAGS=  -lrt -lm 

CFLAGS  = -Wall -Wextra  -g -tcollect -I$(VT_ROOT)/include
FFLAGS  += -tcollect -I$(VT_ROOT)/include
LFLAGS  += -L$(VT_LIB_DIR) -lVT $(VT_ADD_LIBS)

blur:  
	mpicc blurmain.c blurfilter.c ppmio.c gaussw.c subroutines.c -o blurmpi $(LFLAGS)

thresc:
	mpicc thresmain.c ppmio.c -o thresmpi $(LFLAGS) $(CFLAGS) $(FFLAGS)
clean:
	rm blurmpi thresmpi

debug: LFLAGS += -g -DDEBUG -DDBG
debug: blur

runblur:	
	mpirun -np $(p) blurmpi 50  images/im2.ppm images/im2test.ppm

runthres:
	mpirun -np $(p) thresmpi images/im2.ppm images/im2test.ppm
#The run target is used for testing with the paramters given above
