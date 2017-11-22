# Makefile example for compiling cuda and linking cuda to cpp:

SOURCELOC = 

UTILITYLOC =

NEWMOD =

PROGRAM = testGTS

INCDIR= .
#
# Define the C compile flags
CCFLAGS =  -g -O2 -m64 -I ./header -std=c++11
CC = g++



# Define all object files

OBJECTS = \
	Cell.o\
	DateTime.o\
	Schedular.o\
	Grid.o\
	MBB.o\
	PreProcess.o\
	QueryResult.o\
	SamplePoint.o\
	Trajectory.o\
	BufferManager.o\
	GTS.o



# Define Task Function Program


all: testGTS


# Define what Modtools is


testGTS: $(OBJECTS)
	gcc -m64 -c -g p_mmap.c -o p_mmap.o
	$(CC) $(CCFLAGS) -o testGTS  p_mmap.o $(OBJECTS)

# Modtools_Object codes


.cpp.o:

	$(CC) $(CCFLAGS) -c $<


clean:
	rm -rf *.o
#  end

