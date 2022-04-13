CC = g++
FLAGS = -std=c++11
INCLUDES = ../socklib
LIBS = -Llib
MAIN_FILES = ExchangeClient.cpp

all:
	$(RM) ExchangeClient
	
	$(CC) $(FLAGS) $(MAIN_FILES) -o ExchangeClient

	printf "ExchangeClient build complete..\n"
	printf "\n"

clean:
	$(RM) ExchangeClient

.SILENT: all test clean
.PHONY: all test clean

