CLPATH=/opt/intel/opencl-sdk
CC=gcc
CFLAGS=-Wall -O2 -I$(CLPATH)/include -D_VERBOSE
LDFLAGS=-lm -pthread -lOpenCL
SOURCES=approx.c database.c evo.c load_ini.c main.c mwc64x_rng.c opencl_target.c
EXECUTABLE=glucose

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECUTABLE) $(LDFLAGS) 
	strip $(EXECUTABLE)

clean:
	rm glucose
