CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g -lpthread

SRC = src/main.cpp src/grafo.cpp src/scheduler.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o planificador

clean:
	rm -f planificador
