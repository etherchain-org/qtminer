ETH = $(shell realpath ../genoil-ethereum-master)
ETHLIBS = ethash ethash-cl ethcore devcore

CC = g++
CFLAGS = -Wall -g -std=c++11 -fPIC
INCLUDES = -I/usr/local/include -I$(ETH) $(shell pkg-config --cflags Qt5Network)
LFLAGS = $(foreach path,$(foreach lib,$(ETHLIBS),$(ETH)/build/lib$(lib)),-Wl,-L$(path) -Wl,-rpath,$(path)) -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib
LIBS = $(foreach lib,$(ETHLIBS),-l$(lib)) -lOpenCL -lboost_filesystem -lboost_system -lboost_thread $(shell pkg-config --libs Qt5Network)

.ONESHELL:

all: qtminer

qtminer:  qtminer.o main.o QtMiner.moc.o
	$(CC) $(CFLAGS) -o qtminer qtminer.o main.o QtMiner.moc.o $(LFLAGS) $(LIBS)
	echo '#!/bin/sh' > qtminer.sh
	cat >> qtminer.sh <<DONE
	export GPU_FORCE_64BIT_PTR=1
	export GPU_MAX_HEAP_SIZE=100
	export GPU_USE_SYNC_OBJECTS=1
	export GPU_MAX_ALLOC_PERCENT=100
	export GPU_SINGLE_ALLOC_PERCENT=100
	LD_LIBRARY_PATH=$(ETH)/build/libethash \$$(dirname \$$0)/qtminer \$$*
	DONE
	chmod a+x qtminer.sh

QtMiner.moc.cpp: QtMiner.h
	moc -o QtMiner.moc.cpp QtMiner.h

QtMiner.moc.o: QtMiner.moc.cpp QtMiner.h
	$(CC) $(CFLAGS) $(INCLUDES) -c QtMiner.moc.cpp

qtminer.o:  qtminer.cpp QtMiner.h
	$(CC) $(CFLAGS) $(INCLUDES) -c qtminer.cpp

main.o:  main.cpp QtMiner.h
	$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp

clean:
	$(RM) qtminer *.o *~
