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
CXXFLAGS += -g -Wall -Wextra -pthread
CFLAGS += -g

$(FPTREE) : fptree.cpp $(FPTREE_HEADER_DIR)/fptree.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR)

$(UTILITY) :utility.cpp $(HEADER_DIR)/utility/utility.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR)

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -o $@

$(CLHASH) : clhash.c $(HEADER_DIR)/utility/clhash.h
	gcc $(CFLAGS) -c $< -I $(HEADER_DIR)

main.o : main.cpp $(FPTREE_HEADER_DIR)/fptree.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -lpmem -lpthread 

lycsb.o : lycsb.cpp
	$(CXX) $(CXXFLAGS) -c $< -lleveldb -lpthread

ycsb.o : ycsb.cpp $(FPTREE_HEADER_DIR)/fptree.h
	$(CXX) $(CXXFLAGS) -c $< -I $(HEADER_DIR) -lpmem -lleveldb -lpthread

$(MAIN) : main.o $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpmem -lpthread 

$(LYCSB) : lycsb.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -lleveldb -lpthread

$(YCSB) : ycsb.o $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpmem -lpthread -lleveldb

$(BIN):
	mkdir -p bin

all : $(ALL)

clean :
	rm -rf *.o ./bin/* /mnt/pmemdir/*

cleand : 
	rm -rf /mnt/pmemdir/*
