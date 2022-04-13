CC = g++
FLAGS = -std=c++11
INCLUDES = 
LIBS = -Llib
MAIN_FILES = FileReduce.cpp

all:
	$(RM) FileReduce
	
	$(CC) $(FLAGS) $(MAIN_FILES) -o FileReduce

	printf "FileReduce build complete..\n"
	printf "\n"

clean:
	$(RM) FileReduce

.SILENT: all test clean
.PHONY: all test clean

