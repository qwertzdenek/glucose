CLPATH=/opt/AMDAPP/SDK
CC=gcc
CFLAGS=-Wall -I$(CLPATH)/include -D_VERBOSE
LDFLAGS=-lm -lsqlite3 -pthread -lOpenCL
SOURCES=approx.c database.c evo.c load_ini.c main.c mwc64x_rng.c opencl_target.c
EXECUTABLE=glucose

Release:
	$(CC) -O2 $(CFLAGS) $(SOURCES) -o $(EXECUTABLE) $(LDFLAGS)
	strip $(EXECUTABLE)
Debug:
	$(CC) -g $(CFLAGS) $(SOURCES) -o $(EXECUTABLE) $(LDFLAGS)

clean:
	rm $(EXECUTABLE)

