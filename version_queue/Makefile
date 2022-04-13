CC = g++
FLAGS = -std=c++11
INCLUDES = 
LIBS = -Llib
MAIN_FILES = VersionQueue.cpp

all:
	$(RM) VersionQueue
	
	$(CC) $(FLAGS) $(MAIN_FILES) -o VersionQueue

	printf "VersionQueue build complete..\n"
	printf "\n"

clean:
	$(RM) VersionQueue

.SILENT: all test clean
.PHONY: all test clean

