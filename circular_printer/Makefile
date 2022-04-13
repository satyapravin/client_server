CC = g++
FLAGS = -std=c++11 -pthread
INCLUDES = 
LIBS = -Llib -lpthread
MAIN_FILES = CircularPrinter.cpp

all:
	$(RM) CircularPrinter
	
	$(CC) $(FLAGS) $(MAIN_FILES) -o CircularPrinter

	printf "CircularPrinter build complete..\n"
	printf "\n"

clean:
	$(RM) CircularPrinter

.SILENT: all test clean
.PHONY: all test clean

