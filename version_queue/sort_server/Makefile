CC = g++
FLAGS = -std=c++11
INCLUDES = ../socklib
LIBS = -Llib
MAIN_FILES = SortServer.cpp

all:
	$(RM) SortServer
	
	$(CC) $(FLAGS) $(MAIN_FILES) -o SortServer

	printf "SortServer build complete..\n"
	printf "\n"

clean:
	$(RM) SortServer

.SILENT: all test clean
.PHONY: all test clean

