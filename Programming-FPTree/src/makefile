CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

CXXFLAGS = -std=c++11

CFLAGS = -fPIC -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = bin

FPTREE = fptree.o

UTILITY = utility.o

PALLOCATOR = pallocator.o

CLHASH = clhash.o

ALL := $(LYCSB) $(YCSB) $(MAIN) $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)

#============================TODO: all below============================
$(LYCSB) : lycsb.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -lleveldb -lpthread

lycsb.o : lycsb.cpp
	$(CXX) $(CXXFLAGS) -c lycsb.cpp -g -o $@ -lleveldb -lpthread

$(MAIN) : main.o $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)
	$(CXX) $(CFLAGS) $^ -o $@ -lpmem -lpthread -lleveldb

main.o : main.cpp $(FPTREE_HEADER_DIR)/fptree.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -g -o $@ -lpthread -lleveldb

$(FPTREE) : fptree.cpp $(FPTREE_HEADER_DIR)/fptree.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -g -o $@

$(UTILITY) :utility.cpp $(HEADER_DIR)/utility/utility.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -g -o $@

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -g -o $@

$(CLHASH) : clhash.c $(HEADER_DIR)/utility/clhash.h
	gcc $(CFLAGS) -c clhash.c -I $(HEADER_DIR) -g -o $@

$(BIN):
	mkdir -p bin

all : $(ALL)

clean :
	rm -rf *.o ./bin/* ../data/*

cleand : 
	rm -rf ../data/*
