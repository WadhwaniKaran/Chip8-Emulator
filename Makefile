#CC=g++
#cflags=-c -Wall 

#all: prog

#prog: main.o cpu.o memory.o

#main.o: main.cpp
#	$(CC) $(cflags) main.cpp `sdl2-config --cflags --libs`

#cpu.o: cpu.cpp
#	$(CC) $(cflags) cpu.cpp

#memory.o: memory.cpp
#	$(CC) $(cflags) memory.cpp

#clean:
#	rm -rf *.o

OBJS = main.cpp cpu.cpp memory.cpp
all: $(OBJS)
	g++ -o main $(OBJS) `sdl2-config --cflags --libs` -lSDL2_ttf
